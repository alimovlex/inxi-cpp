// colors.cpp
#include "colors.h"
#include <iostream>
#include <unistd.h>

namespace inxi {

static const std::vector<ColorScheme> ALL_SCHEMES = {
    {COL_EMPTY,   COL_EMPTY,    COL_EMPTY},    // 0: no color
    {COL_DRED,    COL_RED,      COL_GREY},     // 1: red
    {COL_BLUE,    COL_CYAN,     COL_GREY},     // 2: blue (default)
    {COL_CYAN,    COL_DCYAN,    COL_GREY},     // 3: cyan
    {COL_GREEN,   COL_DGREEN,   COL_GREY},     // 4: green
    {COL_MAGENTA, COL_DMAGENTA, COL_GREY},     // 5: magenta
    {COL_RED,     COL_DRED,     COL_GREY},     // 6: light red
    {COL_YELLOW,  COL_DYELLOW,  COL_GREY},     // 7: yellow
    {COL_GREY,    COL_DGREY,    COL_WHITE},    // 8: monochrome
    {COL_BLUE,    COL_GREEN,    COL_CYAN},     // 9: multi 1
    {COL_RED,     COL_YELLOW,   COL_GREEN},    // 10: multi 2
    {COL_CYAN,    COL_MAGENTA,  COL_BLUE},     // 11: multi 3
    {COL_GREEN,   COL_YELLOW,   COL_CYAN},     // 12: multi 4
    {COL_MAGENTA, COL_CYAN,     COL_GREEN},    // 13: multi 5
    {COL_DYELLOW, COL_CYAN,     COL_GREY}      // 14: multi 6
};

std::vector<ColorScheme> get_color_schemes() {
    return ALL_SCHEMES;
}

int get_color_scheme_count() {
    return static_cast<int>(ALL_SCHEMES.size());
}

static std::string ansi_code(const std::string& col_name) {
    if (col_name == COL_NORMAL)   return "\033[0m";
    if (col_name == COL_EMPTY)    return "";
    if (col_name == COL_BLACK)    return "\033[30m";
    if (col_name == COL_DBLUE)    return "\033[34m";
    if (col_name == COL_BLUE)     return "\033[1;34m";
    if (col_name == COL_DCYAN)    return "\033[36m";
    if (col_name == COL_CYAN)     return "\033[1;36m";
    if (col_name == COL_DGREEN)   return "\033[32m";
    if (col_name == COL_GREEN)    return "\033[1;32m";
    if (col_name == COL_DMAGENTA) return "\033[35m";
    if (col_name == COL_MAGENTA)  return "\033[1;35m";
    if (col_name == COL_DRED)     return "\033[31m";
    if (col_name == COL_RED)      return "\033[1;31m";
    if (col_name == COL_WHITE)    return "\033[1;37m";
    if (col_name == COL_GREY)     return "\033[37m";
    if (col_name == COL_DGREY)    return "\033[1;30m";
    if (col_name == COL_DYELLOW)  return "\033[33m";
    if (col_name == COL_YELLOW)   return "\033[1;33m";
    return "";
}

void set_colors(GlobalState& state) {
    bool is_tty = isatty(STDOUT_FILENO) != 0;
    if (!is_tty && state.output_type == "screen" && state.colors.scheme != 0) {
        // Default to no color if piped, unless forced
    }
    set_color_scheme(state, state.colors.scheme);
}

void set_color_scheme(GlobalState& state, int scheme_index) {
    state.colors.scheme = scheme_index;
    if (scheme_index <= 0 || scheme_index >= static_cast<int>(ALL_SCHEMES.size())) {
        state.colors.c1 = "";
        state.colors.c2 = "";
        state.colors.c3 = "";
        state.colors.reset = "";
        return;
    }

    const auto& sc = ALL_SCHEMES[scheme_index];
    state.colors.c1 = ansi_code(sc[0]);
    state.colors.c2 = ansi_code(sc[1]);
    state.colors.c3 = ansi_code(sc[2]);
    state.colors.reset = "\033[0m";
}

SelectColors::SelectColors(GlobalState& state) : state_(state) {}

void SelectColors::select_schema() {
    start_selector();
}

void SelectColors::set_status() {}
void SelectColors::assign_selectors() {}
void SelectColors::start_selector() {}
std::string SelectColors::create_selections() { return ""; }
std::string SelectColors::get_selection() { return ""; }
void SelectColors::process_selection(const std::string& choice) {}
void SelectColors::delete_all() {}
void SelectColors::delete_global() {}
void SelectColors::set_config_scheme(int scheme_index) {}
void SelectColors::print_irc_message() {}

}  // namespace inxi
