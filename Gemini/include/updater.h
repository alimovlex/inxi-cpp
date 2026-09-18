// updater.h
// Self-updater logic for inxi (-U / --update).
#pragma once
#include "global_state.h"
#include <string>

namespace inxi {

class Updater {
public:
    explicit Updater(GlobalState& state);

    bool check_for_updates();
    bool perform_update();

private:
    GlobalState& state_;
};

}  // namespace inxi
