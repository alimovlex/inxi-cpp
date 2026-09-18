// configs.h
// Config file reading / writing.
// Perl source: package Configs (lines ~1173–1531)
//
// Perl subs mapped:
//   Configs::set()          → Configs::load()
//   Configs::show()         → Configs::show()
//   Configs::process_item() → Configs::process_item()
//   Configs::check_file()   → Configs::check_file()
//   Configs::begin_logging()→ Configs::begin_logging()
//   Configs::log_data()     → Configs::log_data()
//   set_debugger()          → Configs::set_debugger()
#pragma once
#include "global_state.h"
#include <string>

namespace inxi {

class Configs {
public:
    explicit Configs(GlobalState& state);

    // Perl: Configs::set() — load and apply all config files
    void load();

    // Perl: Configs::show() — print current config key/values
    void show();

    // Perl: Configs::process_item() — handle a single config key=value
    void process_item(const std::string& key, const std::string& value);

    // Perl: Configs::check_file() — locate config file; returns path or ""
    std::string check_file();

    // Perl: Configs::begin_logging() — open / initialise log file
    void begin_logging();

    // Perl: Configs::log_data() — write a line to the log file
    void log_data(const std::string& tag, const std::string& msg);

    // Perl: set_debugger() — apply debugger level flags from config/CLI
    void set_debugger();

private:
    GlobalState& state_;
    std::string  log_path_;

    // Parse a config file and call process_item for each valid line
    void parse_file(const std::string& path);
};

}  // namespace inxi
