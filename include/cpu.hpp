#pragma once
/* sysmon cpu.hpp — Copyright 2025, Apache-2.0 */
#include <string>
#include <vector>

namespace Cpu {
    extern double              total_usage;
    extern std::vector<double> core_usage;
    extern std::vector<double> history;
    extern std::vector<std::vector<double>> core_history;
    extern std::string         frequency;
    extern std::string         temperature;
    extern std::string         uptime;
    extern std::string         model_name;
    extern int                 num_cores;

    // GPU (optional — populated if nvidia-smi or amdgpu sysfs available)
    extern double      gpu_pct;
    extern std::string gpu_mem_str;
    extern std::string gpu_temp_str;
    extern std::string gpu_usage_str;  // empty = no GPU detected

    void init();
    void collect();
}
