/* sysmon proc.cpp — Copyright 2025, Apache-2.0 */
#include "../include/proc.hpp"
#include "../include/config.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <unordered_map>
#include <pwd.h>
#include <unistd.h>
#include <chrono>

namespace Proc {

std::vector<Process> procs;
std::string  filter;
std::string  sort_by = "cpu lazy";
bool         reverse = false;
bool         tree_view = false;
int          selected = 0;
int          scroll_offset = 0;
int          total_count = 0;

static long long TICK_HZ = 100;
static long long MEM_TOTAL_KB = 1;

struct PrevTimes { long long utime=0, stime=0, ts=0; };
static std::unordered_map<int, PrevTimes> prev;

static long long now_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

static std::string uid_name(uid_t uid) {
    struct passwd* pw=getpwuid(uid);
    return pw ? pw->pw_name : std::to_string(uid);
}

static long long get_mem_total() {
    std::ifstream f("/proc/meminfo"); std::string l;
    while(std::getline(f,l)) {
        if(l.rfind("MemTotal:",0)==0) {
            std::istringstream s(l); std::string k; long long v; s>>k>>v; return v;
        }
    }
    return 1;
}

static bool read_proc(int pid, Process& p) {
    std::string stat_path="/proc/"+std::to_string(pid)+"/stat";
    std::ifstream sf(stat_path); if(!sf) return false;
    std::string line; std::getline(sf,line);

    auto ns=line.find('('), ne=line.rfind(')');
    if(ns==std::string::npos||ne==std::string::npos) return false;
    p.pid=pid;
    p.name=line.substr(ns+1,ne-ns-1);

    std::istringstream rest(line.substr(ne+2));
    std::string state;
    long long ppid,pgrp,sess,tty,tpgid,flags,minflt,cminflt,majflt,cmajflt,utime,stime;
    rest>>state>>ppid>>pgrp>>sess>>tty>>tpgid>>flags
        >>minflt>>cminflt>>majflt>>cmajflt>>utime>>stime;
    p.state=state; p.ppid=(int)ppid;

    long long ts=now_ms();
    long long cur=utime+stime;
    if(prev.count(pid)) {
        auto& pv=prev[pid];
        long long dticks=cur-pv.utime-pv.stime;
        long long dms=ts-pv.ts;
        if(dms>0) p.cpu_pct=std::clamp(100.0*dticks*1000.0/(TICK_HZ*dms),0.0,
                                        (double)(sysconf(_SC_NPROCESSORS_ONLN)*100));
    }
    prev[pid]={utime,stime,ts};

    // memory
    std::ifstream smf("/proc/"+std::to_string(pid)+"/statm");
    if(smf){
        long long pages; smf>>pages;
        p.mem_bytes=pages*sysconf(_SC_PAGESIZE);
        p.mem_pct=MEM_TOTAL_KB>0 ? 100.0*p.mem_bytes/(MEM_TOTAL_KB*1024) : 0.0;
    }

    // user + threads from /proc/<pid>/status
    std::ifstream stf("/proc/"+std::to_string(pid)+"/status");
    std::string sl;
    while(std::getline(stf,sl)) {
        if(sl.rfind("Uid:",0)==0) {
            std::istringstream s(sl); std::string k; uid_t uid; s>>k>>uid;
            p.user=uid_name(uid);
        } else if(sl.rfind("Threads:",0)==0) {
            std::istringstream s(sl); std::string k; int t; s>>k>>t;
            p.threads=t;
        }
    }

    // full command line from /proc/<pid>/cmdline
    {
        std::ifstream cf("/proc/"+std::to_string(pid)+"/cmdline");
        std::string raw((std::istreambuf_iterator<char>(cf)),
                         std::istreambuf_iterator<char>());
        // cmdline is NUL-separated argv; replace NULs with spaces
        for(auto& c : raw) if(c=='\0') c=' ';
        while(!raw.empty() && raw.back()==' ') raw.pop_back();
        p.cmd = raw.empty() ? p.name : raw;
    }

    return true;
}

void init(const std::string& f) {
    filter=f;
    sort_by=Config::str("proc_sorting");
    reverse=Config::boolean("proc_reversed");
    tree_view=Config::boolean("proc_tree");
    TICK_HZ=sysconf(_SC_CLK_TCK);
    MEM_TOTAL_KB=get_mem_total();
}

void collect() {
    MEM_TOTAL_KB=get_mem_total();
    std::vector<Process> collected;

    for(auto& entry:std::filesystem::directory_iterator("/proc")) {
        if(!entry.is_directory()) continue;
        std::string name=entry.path().filename().string();
        if(name.empty()||!std::isdigit(name[0])) continue;
        int pid; try{pid=std::stoi(name);}catch(...){continue;}
        Process p;
        if(!read_proc(pid,p)) continue;
        if(!filter.empty()&&p.name.find(filter)==std::string::npos) continue;
        collected.push_back(p);
    }

    total_count=(int)collected.size();

    std::sort(collected.begin(),collected.end(),[](const Process& a,const Process& b){
        return a.cpu_pct>b.cpu_pct; // default: cpu lazy
    });
    if(reverse) std::reverse(collected.begin(),collected.end());

    scroll_offset=std::clamp(scroll_offset,0,std::max(0,(int)collected.size()-1));
    if(scroll_offset>0&&scroll_offset<(int)collected.size())
        collected.erase(collected.begin(),collected.begin()+scroll_offset);

    procs=std::move(collected);
}

} // namespace Proc
