// colors.h
// Terminal color / ANSI escape code management.
// Perl subs mapped:
//   get_color_scheme()        → Colors::get_scheme()
//   set_color_scheme()        → Colors::set_scheme()
//   set_colors()              → Colors::set()
//   SelectColors::new()       → SelectColors::SelectColors()
//   SelectColors::select_schema()   → SelectColors::select_schema()
//   SelectColors::set_status()      → SelectColors::set_status()
//   SelectColors::assign_selectors()→ SelectColors::assign_selectors()
//   SelectColors::start_selector()  → SelectColors::start_selector()
//   SelectColors::create_color_selections() → SelectColors::create_selections()
//   SelectColors::get_selection()   → SelectColors::get_selection()
//   SelectColors::process_selection()→SelectColors::process_selection()
//   SelectColors::delete_all_colors()→SelectColors::delete_all()
//   SelectColors::delete_global_color()→SelectColors::delete_global()
//   SelectColors::set_config_color_scheme()→SelectColors::set_config_scheme()
//   SelectColors::print_irc_message()→SelectColors::print_irc_message()
#pragma once
#include "global_state.h"
#include <array>
#include <string>
#include <vector>

namespace inxi {

// ANSI color name constants (indices into ColorState::ansi)
// These mirror the ALL-CAPS names in the Perl color_schemes table.
constexpr const char* COL_NORMAL   = "NORMAL";
constexpr const char* COL_EMPTY    = "EMPTY";
constexpr const char* COL_BLACK    = "BLACK";
constexpr const char* COL_DBLUE    = "DBLUE";
constexpr const char* COL_BLUE     = "BLUE";
constexpr const char* COL_CYAN     = "CYAN";
constexpr const char* COL_DCYAN    = "DCYAN";
constexpr const char* COL_GREEN    = "GREEN";
constexpr const char* COL_DGREEN   = "DGREEN";
constexpr const char* COL_MAGENTA  = "MAGENTA";
constexpr const char* COL_DMAGENTA = "DMAGENTA";
constexpr const char* COL_RED      = "RED";
constexpr const char* COL_DRED     = "DRED";
constexpr const char* COL_WHITE    = "WHITE";
constexpr const char* COL_GREY     = "GREY";
constexpr const char* COL_DGREY    = "DGREY";
constexpr const char* COL_YELLOW   = "YELLOW";
constexpr const char* COL_DYELLOW  = "DYELLOW";

// One color scheme: three color names [c1, c2, c3]
using ColorScheme = std::array<std::string, 3>;

// Perl: get_color_scheme('full') — returns all defined schemes
std::vector<ColorScheme> get_color_schemes();

// Perl: get_color_scheme('count') — returns scheme count
int get_color_scheme_count();

// Perl: set_colors() — build the ANSI escape map and apply active scheme
void set_colors(GlobalState& state);

// Perl: set_color_scheme(n) — apply scheme index n to state
void set_color_scheme(GlobalState& state, int scheme_index);

// -----------------------------------------------------------------------
// SelectColors — interactive color picker (Perl package SelectColors)
// -----------------------------------------------------------------------
class SelectColors {
public:
    explicit SelectColors(GlobalState& state);

    // Perl: select_schema() — top-level interactive selector entry point
    void select_schema();

    // Perl: set_status() — prints current color status line
    void set_status();

    // Perl: assign_selectors() — assigns keyboard keys to schemes
    void assign_selectors();

    // Perl: start_selector() — begins the interactive loop
    void start_selector();

    // Perl: create_color_selections() — builds the selection display
    std::string create_selections();

    // Perl: get_selection() — reads user input
    std::string get_selection();

    // Perl: process_selection(choice) — applies chosen scheme
    void process_selection(const std::string& choice);

    // Perl: delete_all_colors() — removes all color config
    void delete_all();

    // Perl: delete_global_color() — removes global color scheme config
    void delete_global();

    // Perl: set_config_color_scheme() — writes chosen scheme to config
    void set_config_scheme(int scheme_index);

    // Perl: print_irc_message() — prints IRC-safe color demo
    void print_irc_message();

private:
    [[maybe_unused]] GlobalState& state_;
    [[maybe_unused]] std::vector<std::string> selectors_; // keyboard key → scheme
};

}  // namespace inxi
