/* sysmon net.cpp — Copyright 2025, Apache-2.0 */
#include "../include/net.hpp"
#include "../include/config.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <filesystem>
#include <arpa/inet.h>
#include <ifaddrs.h>

namespace Net {

std::string interface = "lo";
long long   rx_speed{}, tx_speed{};
long long   rx_total{}, tx_total{};
long long   rx_top{},   tx_top{};
std::string rx_str="0 B/s", tx_str="0 B/s";
std::string rx_total_str="0 B", tx_total_str="0 B";
std::vector<double> rx_history, tx_history;
double rx_max=1000, tx_max=1000;
std::string ip_addr;

static constexpr size_t MAX_HIST=300;

struct IfStat { long long rx=0, tx=0; };
static std::unordered_map<std::string,IfStat> prev_stat;
static std::chrono::steady_clock::time_point prev_time;
static bool first=true;

static std::string fmt_speed(long long bps) {
    std::ostringstream o;
    if     (bps>=1LL<<30) o<<std::fixed<<std::setprecision(1)<<(double)bps/(1LL<<30)<<" GiB/s";
    else if(bps>=1LL<<20) o<<std::fixed<<std::setprecision(1)<<(double)bps/(1LL<<20)<<" MiB/s";
    else if(bps>=1LL<<10) o<<std::fixed<<std::setprecision(1)<<(double)bps/(1LL<<10)<<" KiB/s";
    else                  o<<bps<<" B/s";
    return o.str();
}
static std::string fmt_bytes(long long b) {
    std::ostringstream o;
    if     (b>=1LL<<30) o<<std::fixed<<std::setprecision(2)<<(double)b/(1LL<<30)<<" GiB";
    else if(b>=1LL<<20) o<<std::fixed<<std::setprecision(1)<<(double)b/(1LL<<20)<<" MiB";
    else if(b>=1LL<<10) o<<std::fixed<<std::setprecision(0)<<(double)b/(1LL<<10)<<" KiB";
    else                o<<b<<" B";
    return o.str();
}

static std::string detect_iface() {
    std::ifstream f("/proc/net/dev"); std::string line;
    std::getline(f,line); std::getline(f,line); // skip headers
    std::string best; long long best_rx=-1;
    while(std::getline(f,line)) {
        auto p=line.find(':'); if(p==std::string::npos) continue;
        std::string name=line.substr(0,p);
        size_t a=name.find_first_not_of(" \t");
        if(a!=std::string::npos) name=name.substr(a);
        if(name=="lo") continue;
        std::istringstream s(line.substr(p+1)); long long rx; s>>rx;
        if(rx>best_rx){best_rx=rx; best=name;}
    }
    return best.empty()?"lo":best;
}

static bool read_iface(const std::string& iface, IfStat& s) {
    std::ifstream f("/proc/net/dev"); std::string line;
    while(std::getline(f,line)) {
        auto p=line.find(':'); if(p==std::string::npos) continue;
        std::string name=line.substr(0,p);
        size_t a=name.find_first_not_of(" \t");
        if(a!=std::string::npos) name=name.substr(a);
        if(name!=iface) continue;
        std::istringstream ss(line.substr(p+1));
        long long j;
        ss>>s.rx>>j>>j>>j>>j>>j>>j>>j>>s.tx;
        return true;
    }
    return false;
}

static std::string get_ip(const std::string& iface) {
    struct ifaddrs* ifa=nullptr;
    if(getifaddrs(&ifa)!=0) return "";
    std::string ip;
    for(auto* p=ifa;p;p=p->ifa_next) {
        if(!p->ifa_addr||!p->ifa_name) continue;
        if(std::string(p->ifa_name)!=iface) continue;
        if(p->ifa_addr->sa_family==AF_INET) {
            char buf[INET_ADDRSTRLEN];
            inet_ntop(AF_INET,&((struct sockaddr_in*)p->ifa_addr)->sin_addr,buf,sizeof(buf));
            ip=buf; break;
        }
    }
    if(ifa) freeifaddrs(ifa);
    return ip;
}

void init() {
    std::string cfg=Config::str("net_iface");
    interface=cfg.empty()?detect_iface():cfg;
    prev_time=std::chrono::steady_clock::now();
    IfStat s; if(read_iface(interface,s)) prev_stat[interface]=s;
    ip_addr=get_ip(interface);
    first=true;
}

void collect() {
    auto now=std::chrono::steady_clock::now();
    double dt=std::chrono::duration<double>(now-prev_time).count();
    if(dt<0.01) dt=0.01;
    prev_time=now;

    IfStat cur;
    if(!read_iface(interface,cur)) return;

    if(!first&&prev_stat.count(interface)) {
        auto& p=prev_stat[interface];
        rx_speed=std::max(0LL,(long long)((cur.rx-p.rx)/dt));
        tx_speed=std::max(0LL,(long long)((cur.tx-p.tx)/dt));
    }
    first=false;
    prev_stat[interface]=cur;
    rx_total=cur.rx; tx_total=cur.tx;
    if(rx_speed>rx_top) rx_top=rx_speed;
    if(tx_speed>tx_top) tx_top=tx_speed;

    rx_str=fmt_speed(rx_speed); tx_str=fmt_speed(tx_speed);
    rx_total_str=fmt_bytes(rx_total); tx_total_str=fmt_bytes(tx_total);

    // normalize to % for graph (auto-scaling)
    if(rx_speed>rx_max) rx_max=rx_speed*1.2;
    if(tx_speed>tx_max) tx_max=tx_speed*1.2;
    // slowly decay max
    rx_max=std::max(rx_max*0.99, 1000.0);
    tx_max=std::max(tx_max*0.99, 1000.0);

    double rxp=std::min(100.0, 100.0*rx_speed/rx_max);
    double txp=std::min(100.0, 100.0*tx_speed/tx_max);

    rx_history.push_back(rxp);
    tx_history.push_back(txp);
    if(rx_history.size()>MAX_HIST) rx_history.erase(rx_history.begin());
    if(tx_history.size()>MAX_HIST) tx_history.erase(tx_history.begin());

    // refresh IP occasionally
    static int tick=0;
    if(++tick%30==0) ip_addr=get_ip(interface);
}

} // namespace Net
