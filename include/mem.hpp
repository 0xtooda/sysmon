#pragma once
/* sysmon mem.hpp — Copyright 2025, Apache-2.0 */
#include <string>
#include <vector>

namespace Mem {
    struct Disk {
        std::string mount;
        std::string fs_type;
        long long total{}, used{}, free{};
        double    used_pct{};
    };

    extern long long ram_total, ram_used, ram_free, ram_cached, ram_buffers, ram_available;
    extern double    ram_pct, cached_pct, available_pct;
    extern long long swap_total, swap_used, swap_free;
    extern double    swap_pct;
    extern std::vector<Disk>   disks;
    extern std::vector<double> ram_history, swap_history;

    std::string fmt_bytes(long long b);
    void init();
    void collect();
}
