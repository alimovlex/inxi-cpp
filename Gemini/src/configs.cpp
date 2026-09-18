// configs.cpp
#include "configs.h"
#include "colors.h"
#include "utils/file_utils.h"
#include "utils/string_utils.h"
#include <fstream>
#include <iostream>
#include <unistd.h>

namespace inxi {

Configs::Configs(GlobalState& state) : state_(state) {}

std::string Configs::check_file() {
    if (!state_.user_config_file.empty() && access(state_.user_config_file.c_str(), R_OK) == 0) {
        return state_.user_config_file;
    }
    if (access("/etc/inxi.conf", R_OK) == 0) {
        return "/etc/inxi.conf";
    }
    return "";
}

void Configs::load() {
    std::string path = check_file();
    if (!path.empty()) {
        parse_file(path);
    }
}

void Configs::parse_file(const std::string& path) {
    auto conf = utils::load_config(path);
    for (const auto& [k, v] : conf) {
        process_item(k, v);
    }
}

void Configs::process_item(const std::string& key, const std::string& value) {
    if (key == "COLOR_SCHEME" && utils::is_int(value)) {
        int sc = std::stoi(value);
        state_.colors.scheme = sc;
        set_color_scheme(state_, sc);
    } else if (key == "CPU_SLEEP" && utils::is_numeric(value)) {
        state_.cpu_sleep = std::stod(value);
    } else if (key == "WEATHER_SOURCE" && utils::is_int(value)) {
        state_.weather_source = std::stoi(value);
    } else if (key == "WEATHER_UNIT") {
        state_.weather_unit = value;
    }
}

void Configs::show() {
    std::cout << "Configuration File: " << check_file() << "\n"
              << "COLOR_SCHEME=" << state_.colors.scheme << "\n"
              << "CPU_SLEEP=" << state_.cpu_sleep << "\n"
              << "WEATHER_UNIT=" << state_.weather_unit << "\n";
}

void Configs::begin_logging() {}

void Configs::log_data(const std::string& tag, const std::string& msg) {
    (void)tag;
    (void)msg;
}

void Configs::set_debugger() {}

}  // namespace inxi
