// updater.cpp
#include "updater.h"
#include <iostream>

namespace inxi {

Updater::Updater(GlobalState& state) : state_(state) {}

bool Updater::check_for_updates() {
    std::cout << "Checking for updates (current version: " << state_.self_version << ")...\n";
    return true;
}

bool Updater::perform_update() {
    std::cout << "Updating inxi...\n";
    return true;
}

}  // namespace inxi
