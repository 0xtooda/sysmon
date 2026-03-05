/* sysmon theme.cpp — Copyright 2025, Apache-2.0 */
#include "../include/theme.hpp"
#include "../include/config.hpp"
#include <sstream>
#include <filesystem>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <vector>
#include <iostream>

namespace Theme {

std::unordered_map<std::string, Color> colors;
bool truecolor = true;
bool tty_mode  = false;

static std::string esc_fg(int r, int g, int b) {
    return "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
}
static std::string esc_bg(int r, int g, int b) {
    return "\033[48;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
}

static void set_defaults() {
    // sysmon default theme colors
    colors["main_bg"]          = {0x14, 0x14, 0x14};
    colors["main_fg"]          = {0xcc, 0xcc, 0xcc};
    colors["title"]            = {0x50, 0xc8, 0xff};  // bright cyan
    colors["hi_fg"]            = {0x50, 0xfa, 0x7b};  // bright green
    colors["selected_bg"]      = {0x28, 0x28, 0x50};
    colors["selected_fg"]      = {0xff, 0xff, 0xff};
    colors["inactive_fg"]      = {0x40, 0x40, 0x40};
    colors["graph_text"]       = {0x80, 0x80, 0x80};
    colors["meter_bg"]         = {0x30, 0x30, 0x30};
    colors["proc_misc"]        = {0xc8, 0x96, 0x64};
    // box borders
    colors["cpu_box"]          = {0x50, 0xc8, 0xff};  // cyan
    colors["mem_box"]          = {0x50, 0xfa, 0x7b};  // green
    colors["net_box"]          = {0xff, 0xb8, 0x6c};  // orange
    colors["proc_box"]         = {0xbd, 0x93, 0xf9};  // purple
    colors["div_line"]         = {0x40, 0x40, 0x60};
    // temp gradient
    colors["temp_start"]       = {0x00, 0xc8, 0x50};
    colors["temp_mid"]         = {0xff, 0xb8, 0x6c};
    colors["temp_end"]         = {0xff, 0x55, 0x55};
    // cpu gradient
    colors["cpu_start"]        = {0x50, 0xc8, 0xff};
    colors["cpu_mid"]          = {0x50, 0xfa, 0x7b};
    colors["cpu_end"]          = {0xff, 0x55, 0x55};
    // mem gradients
    colors["free_start"]       = {0xff, 0x55, 0x55};
    colors["free_mid"]         = {0x50, 0xfa, 0x7b};
    colors["free_end"]         = {0x50, 0xfa, 0x7b};  // green = low usage = good
    colors["cached_start"]     = {0x50, 0xfa, 0xc8};
    colors["cached_mid"]       = {0x00, 0xb4, 0x64};
    colors["cached_end"]       = {0x00, 0x64, 0x32};
    colors["available_start"]  = {0xff, 0xe0, 0x00};
    colors["available_mid"]    = {0xc8, 0xb4, 0x00};
    colors["available_end"]    = {0x80, 0x82, 0x00};
    colors["used_start"]       = {0x50, 0xfa, 0x7b};  // green at low
    colors["used_mid"]         = {0xff, 0xb8, 0x6c};  // orange in middle
    colors["used_end"]         = {0xff, 0x55, 0x55};  // red when high
    // net gradients
    colors["download_start"]   = {0x50, 0xc8, 0xff};
    colors["download_mid"]     = {0x00, 0xdc, 0x64};
    colors["download_end"]     = {0x00, 0x64, 0x32};
    colors["upload_start"]     = {0xff, 0xb8, 0x6c};
    colors["upload_mid"]       = {0xff, 0xe0, 0x00};
    colors["upload_end"]       = {0x82, 0xdc, 0x00};
    // process
    colors["process_start"]    = {0x96, 0xdc, 0x00};
    colors["process_mid"]      = {0x00, 0x96, 0x64};
    colors["process_end"]      = {0x00, 0x50, 0xc8};
}

void init() {
    truecolor = Config::boolean("truecolor");
    tty_mode  = Config::boolean("force_tty");
    set_defaults();
    load(Config::str("color_theme"));
}

void load(const std::string& name) {
    if (name == "Default" || name.empty()) { set_defaults(); return; }

    std::vector<std::filesystem::path> paths = {
        std::filesystem::path(Config::conf_dir) / "themes" / (name+".theme"),
        std::filesystem::path("/usr/local/share/sysmon/themes") / (name+".theme"),
        std::filesystem::path("/usr/share/sysmon/themes") / (name+".theme"),
    };
    for (auto& p : paths) {
        if (!std::filesystem::exists(p)) continue;
        std::ifstream f(p);
        std::string line;
        while (std::getline(f, line)) {
            if (line.empty() || line[0]=='#') continue;
            auto eq = line.find('=');
            if (eq==std::string::npos) continue;
            std::string key=line.substr(0,eq), val=line.substr(eq+1);
            auto trim=[](std::string& s){
                size_t a=s.find_first_not_of(" \t\"");
                size_t b=s.find_last_not_of(" \t\"");
                if(a==std::string::npos) s=""; else s=s.substr(a,b-a+1);
            };
            trim(key); trim(val);
            if (val.size()==6) {
                try {
                    int r=std::stoi(val.substr(0,2),nullptr,16);
                    int g=std::stoi(val.substr(2,2),nullptr,16);
                    int b=std::stoi(val.substr(4,2),nullptr,16);
                    colors[key]={r,g,b};
                } catch(...) {}
            }
        }
        return;
    }
}

std::string fg(const std::string& name) {
    if (!truecolor || tty_mode) return "";
    auto it = colors.find(name);
    if (it==colors.end()) return "";
    return esc_fg(it->second.r, it->second.g, it->second.b);
}

std::string bg(const std::string& name) {
    if (!truecolor || tty_mode) return "";
    auto it = colors.find(name);
    if (it==colors.end()) return "";
    return esc_bg(it->second.r, it->second.g, it->second.b);
}

std::string gradient(const std::string& c1, const std::string& c2, double t) {
    if (!truecolor || tty_mode) return "";
    auto a = colors.find(c1), b = colors.find(c2);
    if (a==colors.end()||b==colors.end()) return "";
    t = std::clamp(t, 0.0, 1.0);
    int r = (int)std::round(a->second.r + (b->second.r - a->second.r)*t);
    int g = (int)std::round(a->second.g + (b->second.g - a->second.g)*t);
    int bv= (int)std::round(a->second.b + (b->second.b - a->second.b)*t);
    return esc_fg(r,g,bv);
}

std::string reset() { return "\033[0m"; }
std::string bold()  { return "\033[1m"; }

} // namespace Theme
