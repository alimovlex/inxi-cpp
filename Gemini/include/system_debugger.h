// system_debugger.h
// System diagnostics and raw data collector for debugging.
#pragma once
#include "global_state.h"
#include <string>

namespace inxi {

class SystemDebugger {
public:
    explicit SystemDebugger(GlobalState& state);

    void run_diagnostics();
    void dump_system_files();

private:
    GlobalState& state_;
};

}  // namespace inxi
