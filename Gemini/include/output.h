// output.h
// Output rendering: screen / IRC / JSON / XML.
// Perl subs mapped:
//   output_handler()   → Output::handle()
//   output_control()   → Output::control()
//   print_data()       → Output::print_data()
//   print_line()       → Output::print_line()
//   print_basic()      → Output::print_basic()
//   generate_json()    → Output::generate_json()
//   generate_xml()     → Output::generate_xml()
//   make_line()        → Output::make_line()
//   key()              → Output::key()
//   increment_starters()→ Output::increment_starters()
//   message()          → Output::message()   [see also messages.h]
//   get_chip_id()      → Output::get_chip_id()
#pragma once
#include "global_state.h"
#include "items/info_item.h"
#include <string>
#include <vector>

namespace inxi {

class Output {
public:
    explicit Output(GlobalState& state);

    // Perl: output_handler() — top-level dispatch to screen/file/json/xml
    void handle(const std::vector<OutputRow>& rows);

    // Perl: output_control() — manages line wrapping, color injection
    void control(const std::vector<OutputRow>& rows);

    // Perl: print_data() — full formatted output with colors + wrapping
    void print_data(const std::vector<OutputRow>& rows);

    // Perl: print_line() — print a single pre-formatted line
    void print_line(const std::string& line);

    // Perl: print_basic() — simple text output (--help, --version etc.)
    void print_basic(const std::string& text);

    // Perl: generate_json() — render rows as JSON
    std::string generate_json(const std::vector<OutputRow>& rows);

    // Perl: generate_xml() — render rows as XML
    std::string generate_xml(const std::vector<OutputRow>& rows);

    // Perl: make_line() — build a single formatted output string from a row
    std::string make_line(const OutputRow& row);

    // Perl: key() — wrap a key in color/separator
    std::string key(const std::string& k);

    // Perl: increment_starters() — updates the running line prefix counter
    void increment_starters();

    // Perl: get_chip_id() — returns short chip identifier string
    std::string get_chip_id(const std::string& raw_id);

private:
    GlobalState& state_;
    int          starter_count_ = 0;

    std::string apply_colors(const std::string& s,
                              const std::string& role); // c1/c2/c3/reset
    std::string wrap_line(const std::string& s, int max_width,
                          int indent);
};

}  // namespace inxi
