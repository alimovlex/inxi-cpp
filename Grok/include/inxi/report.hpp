#pragma once

#include "inxi/options.hpp"
#include "inxi/output.hpp"

namespace inxi {

Report build_report(const Options& opt);
void print_recommends();

}  // namespace inxi
