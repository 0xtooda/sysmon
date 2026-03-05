#pragma once
/* sysmon proc.hpp — Copyright 2025, Apache-2.0 */
#include <string>
#include <vector>

namespace Proc {
    struct Process {
        int         pid{};
        int         ppid{};
        std::string name;
        std::string cmd;
        std::string user;
        std::string state;
        double      cpu_pct{};
        double      mem_pct{};
        long long   mem_bytes{};
        int         threads{};
    };

    extern std::vector<Process> procs;
    extern std::string  filter;
    extern std::string  sort_by;
    extern bool         reverse;
    extern bool         tree_view;
    extern int          selected;
    extern int          scroll_offset;
    extern int          total_count;

    void init(const std::string& filter = "");
    void collect();
}
