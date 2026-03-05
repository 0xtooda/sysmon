/* sysmon cpu.cpp — Copyright 2025, Apache-2.0 */
#include "../include/cpu.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <iomanip>
#include <unistd.h>

namespace Cpu {

double              total_usage{};
std::vector<double> core_usage;
std::vector<double> history;
std::vector<std::vector<double>> core_history;
std::string         frequency = "N/A";
std::string         temperature = "N/A";
std::string         uptime = "0:00:00";
std::string         model_name = "Unknown CPU";
int                 num_cores = 1;
double      gpu_pct{};
std::string gpu_mem_str;
std::string gpu_temp_str;
std::string gpu_usage_str;

static constexpr size_t MAX_HIST = 300;

struct Times { long long user,nice,sys,idle,iowait,irq,sirq,steal; };
static std::vector<Times> prev;

static bool read_stat(std::vector<Times>& v) {
    std::ifstream f("/proc/stat");
    if (!f) return false;
    v.clear();
    std::string line;
    while (std::getline(f,line)) {
        if (line.rfind("cpu",0)!=0) continue;
        if (!std::isdigit(line[3]) && line[3]!=' ') continue;
        std::istringstream s(line);
        std::string lbl; Times t{};
        s>>lbl>>t.user>>t.nice>>t.sys>>t.idle>>t.iowait>>t.irq>>t.sirq>>t.steal;
        v.push_back(t);
    }
    return !v.empty();
}

static double pct(const Times& c, const Times& p) {
    long long dtot = (c.user+c.nice+c.sys+c.idle+c.iowait+c.irq+c.sirq+c.steal)
                   - (p.user+p.nice+p.sys+p.idle+p.iowait+p.irq+p.sirq+p.steal);
    long long dbsy = (c.user+c.nice+c.sys+c.irq+c.sirq+c.steal)
                   - (p.user+p.nice+p.sys+p.irq+p.sirq+p.steal);
    if (dtot<=0) return 0.0;
    return std::clamp(100.0*dbsy/dtot, 0.0, 100.0);
}

void init() {
    // model name
    std::ifstream ci("/proc/cpuinfo");
    std::string line;
    while (std::getline(ci,line)) {
        if (line.rfind("model name",0)==0) {
            auto p=line.find(':');
            if (p!=std::string::npos) {
                model_name=line.substr(p+2);
                while(!model_name.empty()&&(model_name.back()=='\n'||model_name.back()=='\r'||model_name.back()==' '))
                    model_name.pop_back();
            }
            break;
        }
    }
    std::vector<Times> t;
    if (read_stat(t)) prev=t;
    num_cores=std::max(1,(int)prev.size()-1);
    core_usage.assign(num_cores,0.0);
    core_history.resize(num_cores);
}

static std::string fmt_freq(long long khz) {
    double mhz=khz/1000.0;
    std::ostringstream o;
    if(mhz>=1000) o<<std::fixed<<std::setprecision(2)<<mhz/1000.0<<" GHz";
    else          o<<std::fixed<<std::setprecision(0)<<mhz<<" MHz";
    return o.str();
}

void collect() {
    std::vector<Times> cur;
    if (!read_stat(cur)||cur.empty()) return;

    if (!prev.empty()&&cur.size()==prev.size()) {
        total_usage=pct(cur[0],prev[0]);
        for(int i=1;i<(int)cur.size()&&i-1<(int)core_usage.size();i++)
            core_usage[i-1]=pct(cur[i],prev[i]);
    }
    prev=cur;

    history.push_back(total_usage);
    if(history.size()>MAX_HIST) history.erase(history.begin());
    for(int i=0;i<(int)core_usage.size();i++) {
        if((int)core_history.size()<=i) core_history.resize(i+1);
        core_history[i].push_back(core_usage[i]);
        if(core_history[i].size()>MAX_HIST) core_history[i].erase(core_history[i].begin());
    }

    // frequency
    { std::ifstream f("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq");
      if(f){ long long k; f>>k; frequency=fmt_freq(k); }
      else {
        std::ifstream ci("/proc/cpuinfo"); std::string l;
        while(std::getline(ci,l)) {
            if(l.rfind("cpu MHz",0)==0){
                auto p=l.find(':');
                if(p!=std::string::npos) frequency=fmt_freq((long long)(std::stod(l.substr(p+1))*1000));
                break;
            }
        }
      }
    }

    // temperature — try hwmon
    temperature="N/A";
    try {
        for(auto& e:std::filesystem::directory_iterator("/sys/class/hwmon")) {
            std::ifstream nf(e.path()/"name"); std::string n; std::getline(nf,n);
            if(n=="coretemp"||n=="k10temp"||n=="zenpower"||n=="cpu_thermal") {
                for(int i=1;i<=20;i++) {
                    std::ifstream tf(e.path()/("temp"+std::to_string(i)+"_input"));
                    if(!tf) continue;
                    int md; tf>>md;
                    std::ostringstream o;
                    o<<std::fixed<<std::setprecision(1)<<md/1000.0<<"\u00b0C";
                    temperature=o.str();
                    goto done_temp;
                }
            }
        }
    } catch(...) {}
    done_temp:;

    // uptime
    { std::ifstream f("/proc/uptime"); double s=0; f>>s;
      long long t=(long long)s;
      long long d=t/86400; t%=86400;
      long long h=t/3600;  t%=3600;
      long long m=t/60;    t%=60;
      std::ostringstream o;
      if(d>0) o<<d<<"d ";
      o<<std::setw(2)<<std::setfill('0')<<h<<":"
       <<std::setw(2)<<std::setfill('0')<<m<<":"
       <<std::setw(2)<<std::setfill('0')<<t;
      uptime=o.str();
    }

    // ── GPU (try NVML via nvidia-smi, then amdgpu sysfs) ────
    gpu_usage_str.clear();
    // Try nvidia
    {
        FILE* p = popen("nvidia-smi --query-gpu=utilization.gpu,memory.used,memory.total,temperature.gpu --format=csv,noheader,nounits 2>/dev/null", "r");
        if (p) {
            int util=0, mem_u=0, mem_t=0, temp=0;
            if (fscanf(p, "%d, %d, %d, %d", &util, &mem_u, &mem_t, &temp) == 4) {
                gpu_pct = util;
                std::ostringstream ms, ts;
                if (mem_t > 0) {
                    double ug = mem_u/1024.0, tg = mem_t/1024.0;
                    ms << std::fixed << std::setprecision(2) << ug << "/" << std::setprecision(1) << tg << "G";
                }
                ts << temp << "\u00b0C";
                gpu_mem_str  = ms.str();
                gpu_temp_str = ts.str();
                gpu_usage_str = std::to_string(util) + "%";
            }
            pclose(p);
        }
    }
    // Try amdgpu sysfs if nvidia didn't work
    if (gpu_usage_str.empty()) {
        try {
            for (auto& e : std::filesystem::directory_iterator("/sys/class/drm")) {
                std::string name = e.path().filename().string();
                if (name.rfind("card",0) != 0 || name.find('-') != std::string::npos) continue;
                auto gpu_busy = e.path() / "device" / "gpu_busy_percent";
                auto mem_used = e.path() / "device" / "mem_info_vram_used";
                auto mem_tot  = e.path() / "device" / "mem_info_vram_total";
                auto temp_f   = e.path() / "device" / "hwmon";
                std::ifstream bf(gpu_busy);
                if (!bf) continue;
                int busy=0; bf >> busy;
                gpu_pct = busy;
                gpu_usage_str = std::to_string(busy)+"%";
                long long mu=0,mt=0;
                { std::ifstream f(mem_used); if(f) f>>mu; }
                { std::ifstream f(mem_tot);  if(f) f>>mt; }
                if (mt > 0) {
                    std::ostringstream ms;
                    ms << std::fixed << std::setprecision(2) << mu/1073741824.0
                       << "/" << std::setprecision(1) << mt/1073741824.0 << "G";
                    gpu_mem_str = ms.str();
                }
                // temp via hwmon
                try {
                    for (auto& hw : std::filesystem::directory_iterator(temp_f)) {
                        std::ifstream tf(hw.path()/"temp1_input");
                        if (!tf) continue;
                        int mv2=0; tf>>mv2;
                        std::ostringstream ts;
                        ts << std::fixed << std::setprecision(0) << mv2/1000.0 << "\u00b0C";
                        gpu_temp_str = ts.str();
                        break;
                    }
                } catch(...) {}
                break;
            }
        } catch(...) {}
    }

}

} // namespace Cpu
