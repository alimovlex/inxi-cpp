#pragma once
#include <ostream>

namespace mod {

// What real inxi shows when invoked with no arguments at all: one
// condensed line hitting the highlights instead of the full report.
void printDefaultSummary(std::ostream& os, bool color);

} // namespace mod
