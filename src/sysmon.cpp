/* sysmon — system resource monitor
   Copyright 2025 sysmon contributors, Apache-2.0 */

#include <iostream>
#include <string>
#include <signal.h>
#include <locale.h>

#include "../include/sysmon.hpp"
#include "../include/config.hpp"
#include "../include/input.hpp"
#include "../include/draw.hpp"
#include "../include/theme.hpp"
#include "../include/cpu.hpp"
#include "../include/mem.hpp"
#include "../include/net.hpp"
#include "../include/proc.hpp"

namespace Global {
    const std::string version = "1.0.0";
    std::atomic<bool> quitting{false};
    std::atomic<bool> resized{false};
    int term_width{};
    int term_height{};
}

static void sig_handler(int sig) {
    if(sig==SIGINT||sig==SIGTERM) Global::quitting=true;
    else if(sig==SIGWINCH)        Global::resized=true;
}

static void print_help() {
    std::cout <<
        "Usage: sysmon [OPTIONS]\n\n"
        "Options:\n"
        "  -c, --config <file>     Path to config file\n"
        "  -d, --debug             Debug mode\n"
        "  -f, --filter <str>      Initial process filter\n"
        "      --force-utf         Override UTF locale detection\n"
        "  -l, --low-color         256-color mode only\n"
        "  -t, --tty               Force TTY mode\n"
        "      --no-tty            Disable TTY mode\n"
        "  -u, --update <ms>       Update interval in ms\n"
        "      --default-config    Print default config\n"
        "  -h, --help              Show this help\n"
        "  -V, --version           Show version\n";
}

int main(int argc, char** argv) {
    setlocale(LC_ALL, "");

    std::string conf_path, proc_filter;
    bool low_color=false, tty=false, no_tty=false;
    int update_ms=2000;

    for(int i=1;i<argc;i++) {
        std::string a=argv[i];
        if(a=="-h"||a=="--help")         { print_help(); return 0; }
        else if(a=="-V"||a=="--version") { std::cout<<"sysmon "<<Global::version<<"\n"; return 0; }
        else if(a=="--default-config")   { Config::init(); Config::print_default(); return 0; }
        else if(a=="-l"||a=="--low-color") low_color=true;
        else if(a=="-t"||a=="--tty")     tty=true;
        else if(a=="--no-tty")           no_tty=true;
        else if((a=="-c"||a=="--config")&&i+1<argc) conf_path=argv[++i];
        else if((a=="-f"||a=="--filter")&&i+1<argc) proc_filter=argv[++i];
        else if((a=="-u"||a=="--update")&&i+1<argc) update_ms=std::stoi(argv[++i]);
    }

    signal(SIGINT,  sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGWINCH,sig_handler);

    Config::init(conf_path);
    if(low_color)  Config::set("truecolor", false);
    if(tty)        Config::set("force_tty", true);
    if(no_tty)     Config::set("force_tty", false);
    Config::set("update_ms", update_ms);

    Theme::init();
    Draw::init();
    Input::init();
    Cpu::init();
    Mem::init();
    Net::init();
    Proc::init(proc_filter);

    while(!Global::quitting) {
        if(Global::resized) { Draw::resize(); Global::resized=false; }
        Cpu::collect();
        Mem::collect();
        Net::collect();
        Proc::collect();
        Draw::draw_all();
        Input::handle(Config::integer("update_ms"));
    }

    Draw::cleanup();
    Input::cleanup();
    return 0;
}
