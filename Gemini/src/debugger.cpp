// debugger.cpp
#include "debugger.h"
#include <iostream>

namespace inxi {

Debugger::Debugger(GlobalState& state) : state_(state) {}

void Debugger::set_level(int level) {
    state_.debugger.level = level;
}

void Debugger::log(int min_level, const std::string& msg) {
    if (state_.debugger.level >= min_level) {
        std::cerr << "[DEBUG " << min_level << "] " << msg << "\n";
    }
}

void Debugger::dump(const std::string& label, const std::string& data) {
    if (state_.debugger.level > 0) {
        std::cerr << "[DUMP: " << label << "]\n" << data << "\n";
    }
}

}  // namespace inxi
