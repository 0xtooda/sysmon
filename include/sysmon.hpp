#pragma once
/* sysmon sysmon.hpp — Copyright 2025, Apache-2.0 */
#include <string>
#include <atomic>

namespace Global {
    extern const std::string version;
    extern std::atomic<bool> quitting;
    extern std::atomic<bool> resized;
    extern int term_width;
    extern int term_height;
}
