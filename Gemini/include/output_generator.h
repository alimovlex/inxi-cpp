// output_generator.h
// Top-level output orchestrator — decides which InfoItems to run.
// Perl: OutputGenerator::generate() and related helpers
//
// Perl subs mapped:
//   OutputGenerator::generate() → OutputGenerator::generate()
#pragma once
#include "global_state.h"

namespace inxi {

class OutputGenerator {
public:
    explicit OutputGenerator(GlobalState& state);

    // Perl: generate() — iterate over enabled show-flags, collect and print
    void generate();

private:
    GlobalState& state_;
};

}  // namespace inxi
