#pragma once
/* sysmon theme.hpp — Copyright 2025, Apache-2.0 */
#include <string>
#include <unordered_map>

namespace Theme {
    struct Color { int r, g, b; };
    extern std::unordered_map<std::string, Color> colors;
    extern bool truecolor;
    extern bool tty_mode;

    void init();
    void load(const std::string& name);
    std::string fg(const std::string& name);
    std::string bg(const std::string& name);
    std::string gradient(const std::string& c1, const std::string& c2, double t);
    std::string reset();
    std::string bold();
}
