/* sysmon mem.cpp — Copyright 2025, Apache-2.0 */
#include "../include/mem.hpp"
#include "../include/config.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <sys/statvfs.h>
#include <mntent.h>
#include <filesystem>
#include <chrono>

namespace Mem {

long long ram_total{}, ram_used{}, ram_free{}, ram_cached{}, ram_buffers{}, ram_available{};
double    ram_pct{}, cached_pct{}, available_pct{};
long long swap_total{}, swap_used{}, swap_free{};
double    swap_pct{};
std::vector<Disk>   disks;
std::vector<double> ram_history, swap_history;

static constexpr size_t MAX_HIST = 300;

std::string fmt_bytes(long long b) {
    std::ostringstream o;
    if      (b>=1LL<<30) o<<std::fixed<<std::setprecision(2)<<(double)b/(1LL<<30)<<"G";
    else if (b>=1LL<<20) o<<std::fixed<<std::setprecision(1)<<(double)b/(1LL<<20)<<"M";
    else if (b>=1LL<<10) o<<std::fixed<<std::setprecision(0)<<(double)b/(1LL<<10)<<"K";
    else                 o<<b<<"B";
    return o.str();
}

static void read_meminfo() {
    std::ifstream f("/proc/meminfo");
    std::string line; long long sf=0;
    while(std::getline(f,line)) {
        std::istringstream s(line); std::string k; long long v; s>>k>>v;
        if(k=="MemTotal:")      ram_total=v*1024;
        else if(k=="MemFree:")  { sf=v*1024; ram_free=v*1024; }
        else if(k=="MemAvailable:") ram_available=v*1024;
        else if(k=="Buffers:")  ram_buffers=v*1024;
        else if(k=="Cached:")   ram_cached=v*1024;
        else if(k=="SwapTotal:")swap_total=v*1024;
        else if(k=="SwapFree:") swap_free=v*1024;
    }
    (void)sf;
    ram_used=ram_total-ram_available;
    // ram_free set above from MemFree:
    swap_used=swap_total-swap_free;
    if(ram_total>0) {
        ram_pct       =100.0*ram_used    /ram_total;
        cached_pct    =100.0*ram_cached  /ram_total;
        available_pct =100.0*ram_available/ram_total;
    }
    if(swap_total>0) swap_pct=100.0*swap_used/swap_total;
}

static void read_disks() {
    disks.clear();
    if(!Config::boolean("show_disks")) return;
    FILE* m=setmntent("/proc/mounts","r"); if(!m) return;
    struct mntent* e;
    while((e=getmntent(m))!=nullptr) {
        std::string fs=e->mnt_type, mnt=e->mnt_dir;
        if(Config::boolean("only_physical")) {
            static const std::vector<std::string> skip={
                "tmpfs","devtmpfs","sysfs","proc","devpts","cgroup","cgroup2",
                "pstore","efivarfs","bpf","tracefs","securityfs","debugfs",
                "hugetlbfs","mqueue","fusectl","configfs","overlay","squashfs"
            };
            if(std::find(skip.begin(),skip.end(),fs)!=skip.end()) continue;
        }
        struct statvfs sv{};
        if(statvfs(mnt.c_str(),&sv)!=0) continue;
        Disk d;
        d.mount=mnt; d.fs_type=fs;
        d.total=(long long)sv.f_blocks*sv.f_frsize;
        d.free =(long long)sv.f_bavail*sv.f_frsize;
        d.used =d.total-(long long)sv.f_bfree*sv.f_frsize;
        if(d.total>0) d.used_pct=100.0*d.used/d.total;
        disks.push_back(d);
    }
    endmntent(m);
    std::sort(disks.begin(),disks.end(),[](const Disk& a,const Disk& b){return a.mount<b.mount;});
    // deduplicate by total size (different bind mounts of same fs)
    disks.erase(std::unique(disks.begin(),disks.end(),[](const Disk& a,const Disk& b){
        return a.total==b.total&&a.used==b.used;
    }),disks.end());
}

void init() {}

void collect() {
    read_meminfo();
    read_disks();
    ram_history.push_back(ram_pct);
    if(ram_history.size()>MAX_HIST) ram_history.erase(ram_history.begin());
    swap_history.push_back(swap_pct);
    if(swap_history.size()>MAX_HIST) swap_history.erase(swap_history.begin());
}

} // namespace Mem
