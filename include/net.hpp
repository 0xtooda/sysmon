#pragma once
/* sysmon net.hpp — Copyright 2025, Apache-2.0 */
#include <string>
#include <vector>
#include <unordered_map>

namespace Net {
    extern std::string interface;
    extern long long   rx_speed, tx_speed;
    extern long long   rx_total, tx_total;
    extern long long   rx_top,   tx_top;
    extern std::string rx_str, tx_str;
    extern std::string rx_total_str, tx_total_str;
    extern std::vector<double> rx_history, tx_history;
    extern double rx_max, tx_max;
    extern std::string ip_addr;

    void init();
    void collect();
}
