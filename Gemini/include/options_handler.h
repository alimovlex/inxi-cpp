// options_handler.h
// Command-line option parsing and post-processing.
// Perl source: package OptionsHandler (lines ~4794–6375)
//
// Perl subs mapped:
//   OptionsHandler::get()           → OptionsHandler::parse()
//   OptionsHandler::post_process()  → OptionsHandler::post_process()
//   OptionsHandler::process_updater()→OptionsHandler::process_updater()
//   OptionsHandler::show_options()  → OptionsHandler::show_options()
//   OptionsHandler::show_version()  → OptionsHandler::show_version()
#pragma once
#include "global_state.h"
#include <string>
#include <vector>

namespace inxi {

class OptionsHandler {
public:
    explicit OptionsHandler(GlobalState& state);

    // Perl: get() — parse argv, populate state.show / state.force / etc.
    void parse(int argc, char* argv[]);

    // Perl: post_process() — second pass: validate, apply overrides
    void post_process();

    // Perl: process_updater() — handle -U / --update flags
    void process_updater();

    // Perl: show_options() — print the full --help text
    void show_options();

    // Perl: show_version() — print --version output
    void show_version();

private:
    GlobalState& state_;

    // Interprets numeric extra levels (-x, -xx, -xxx → extra=1,2,3)
    void set_extra(int level);

    // Apply color scheme index from CLI
    void apply_color(int scheme);

    // Validate and normalise option combos
    void validate();
};

}  // namespace inxi
