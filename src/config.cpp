/* sysmon config.cpp — Copyright 2025, Apache-2.0 */
#include "../include/config.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>

namespace Config {

std::unordered_map<std::string, Val> conf;
std::filesystem::path conf_dir;
std::filesystem::path conf_file;

static const std::string DEFAULT_CONFIG = R"(#? Config file for sysmon v.1.0.0
color_theme = "Default"
theme_background = true
truecolor = true
force_tty = false
vim_keys = false
rounded_corners = true
terminal_sync = true
graph_symbol = "braille"
shown_boxes = "cpu mem net proc"
update_ms = 2000
proc_sorting = "cpu lazy"
proc_reversed = false
proc_tree = false
proc_colors = true
proc_gradient = true
proc_per_core = false
proc_mem_bytes = true
proc_left = false
proc_filter_kernel = false
proc_aggregate = false
cpu_graph_upper = "Auto"
cpu_graph_lower = "Auto"
cpu_invert_lower = true
cpu_single_graph = false
cpu_bottom = false
show_uptime = true
check_temp = true
cpu_sensor = "Auto"
show_coretemp = true
temp_scale = "celsius"
base_10_sizes = false
show_cpu_freq = true
clock_format = "%X"
background_update = true
custom_cpu_name = ""
disks_filter = ""
mem_graphs = true
mem_below_net = false
show_swap = true
swap_disk = true
show_disks = true
only_physical = true
use_fstab = false
show_io_stat = true
io_mode = false
io_graph_combined = false
net_download = 100
net_upload = 100
net_auto = true
net_sync = true
net_iface = ""
show_battery = true
selected_battery = "Auto"
log_level = "WARNING"
)";

void init(const std::string& path) {
    conf["color_theme"]        = std::string("Default");
    conf["theme_background"]   = true;
    conf["truecolor"]          = true;
    conf["force_tty"]          = false;
    conf["vim_keys"]           = false;
    conf["rounded_corners"]    = true;
    conf["graph_symbol"]       = std::string("braille");
    conf["shown_boxes"]        = std::string("cpu mem net proc");
    conf["update_ms"]          = 2000;
    conf["proc_sorting"]       = std::string("cpu lazy");
    conf["proc_reversed"]      = false;
    conf["proc_tree"]          = false;
    conf["proc_colors"]        = true;
    conf["proc_gradient"]      = true;
    conf["proc_per_core"]      = false;
    conf["proc_mem_bytes"]     = true;
    conf["proc_left"]          = false;
    conf["proc_filter_kernel"] = false;
    conf["proc_aggregate"]     = false;
    conf["cpu_graph_upper"]    = std::string("Auto");
    conf["cpu_graph_lower"]    = std::string("Auto");
    conf["cpu_invert_lower"]   = true;
    conf["cpu_single_graph"]   = false;
    conf["cpu_bottom"]         = false;
    conf["show_uptime"]        = true;
    conf["check_temp"]         = true;
    conf["cpu_sensor"]         = std::string("Auto");
    conf["show_coretemp"]      = true;
    conf["temp_scale"]         = std::string("celsius");
    conf["base_10_sizes"]      = false;
    conf["show_cpu_freq"]      = true;
    conf["clock_format"]       = std::string("%X");
    conf["background_update"]  = true;
    conf["custom_cpu_name"]    = std::string("");
    conf["disks_filter"]       = std::string("");
    conf["mem_graphs"]         = true;
    conf["mem_below_net"]      = false;
    conf["show_swap"]          = true;
    conf["swap_disk"]          = true;
    conf["show_disks"]         = true;
    conf["only_physical"]      = true;
    conf["use_fstab"]          = false;
    conf["show_io_stat"]       = true;
    conf["io_mode"]            = false;
    conf["net_download"]       = 100;
    conf["net_upload"]         = 100;
    conf["net_auto"]           = true;
    conf["net_sync"]           = true;
    conf["net_iface"]          = std::string("");
    conf["show_battery"]       = true;
    conf["selected_battery"]   = std::string("Auto");
    conf["log_level"]          = std::string("WARNING");

    if (!path.empty()) { conf_file = path; conf_dir = conf_file.parent_path(); }
    else {
        const char* xdg  = std::getenv("XDG_CONFIG_HOME");
        const char* home = std::getenv("HOME");
        if (xdg)  conf_dir = std::filesystem::path(xdg) / "sysmon";
        else if (home) conf_dir = std::filesystem::path(home) / ".config" / "sysmon";
        else conf_dir = std::filesystem::path("~/.config/sysmon");
        conf_file = conf_dir / "sysmon.conf";
    }
    if (!std::filesystem::exists(conf_dir))
        std::filesystem::create_directories(conf_dir);

    if (std::filesystem::exists(conf_file)) {
        std::ifstream f(conf_file);
        std::string line;
        while (std::getline(f, line)) {
            if (line.empty()||line[0]=='#') continue;
            auto eq = line.find('=');
            if (eq==std::string::npos) continue;
            std::string k=line.substr(0,eq), v=line.substr(eq+1);
            auto trim=[](std::string& s){
                size_t a=s.find_first_not_of(" \t\"");
                size_t b=s.find_last_not_of(" \t\"");
                if(a==std::string::npos)s=""; else s=s.substr(a,b-a+1);
            };
            trim(k); trim(v);
            if (!conf.count(k)) continue;
            if (std::holds_alternative<bool>(conf[k]))        conf[k]=(v=="true");
            else if (std::holds_alternative<int>(conf[k]))    { try{conf[k]=std::stoi(v);}catch(...){} }
            else                                               conf[k]=v;
        }
    } else { save(); }
}

void save() {
    std::ofstream f(conf_file);
    if (f) f << DEFAULT_CONFIG;
}

void print_default() { std::cout << DEFAULT_CONFIG; }

void set(const std::string& k, Val v) { conf[k]=v; }

std::string str(const std::string& k) {
    auto it=conf.find(k);
    if(it==conf.end()) return "";
    if(auto* v=std::get_if<std::string>(&it->second)) return *v;
    return "";
}
bool boolean(const std::string& k) {
    auto it=conf.find(k);
    if(it==conf.end()) return false;
    if(auto* v=std::get_if<bool>(&it->second)) return *v;
    return false;
}
int integer(const std::string& k) {
    auto it=conf.find(k);
    if(it==conf.end()) return 0;
    if(auto* v=std::get_if<int>(&it->second)) return *v;
    return 0;
}

} // namespace Config
