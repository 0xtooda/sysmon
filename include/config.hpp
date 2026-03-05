#pragma once
/* sysmon config.hpp — Copyright 2025, Apache-2.0 */
#include <string>
#include <variant>
#include <unordered_map>
#include <filesystem>

namespace Config {
    using Val = std::variant<std::string, bool, int>;
    extern std::unordered_map<std::string, Val> conf;
    extern std::filesystem::path conf_dir;
    extern std::filesystem::path conf_file;

    void init(const std::string& path = "");
    void save();
    void print_default();
    void set(const std::string& k, Val v);
    std::string str(const std::string& k);
    bool        boolean(const std::string& k);
    int         integer(const std::string& k);
}
