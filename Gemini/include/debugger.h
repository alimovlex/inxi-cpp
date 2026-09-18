// debugger.h
// Debug logging, flags, and timing diagnostics.
#pragma once
#include "global_state.h"
#include <string>

namespace inxi {

class Debugger {
public:
    explicit Debugger(GlobalState& state);

    void set_level(int level);
    void log(int min_level, const std::string& msg);
    void dump(const std::string& label, const std::string& data);

private:
    GlobalState& state_;
};

}  // namespace inxi
