// check_recommends.cpp
#include "check_recommends.h"
#include "utils/process_utils.h"
#include <iomanip>
#include <iostream>

namespace inxi {

CheckRecommends::CheckRecommends(GlobalState& state) : state_(state) {}

std::vector<RecommendedTool> CheckRecommends::check() {
    std::vector<RecommendedTool> recs = {
        {"dmidecode", "Motherboard, BIOS, RAM slots", false, "dmidecode"},
        {"lspci", "PCI bus devices (GPU, Audio, Network)", false, "pciutils"},
        {"lsblk", "Block devices and storage layout", false, "util-linux"},
        {"sensors", "Hardware temperature and fan sensors", false, "lm_sensors"},
        {"smartctl", "Disk drive health, temperatures", false, "smartmontools"},
        {"ip", "Network IP configuration", false, "iproute2"},
        {"bluetoothctl", "Bluetooth status and devices", false, "bluez"},
        {"curl", "Weather / WAN IP downloader", false, "curl"},
        {"upower", "Battery state on laptops", false, "upower"}
    };

    for (auto& r : recs) {
        r.installed = !utils::check_program(r.name, state_.paths).empty();
    }
    return recs;
}

void CheckRecommends::print_report() {
    auto recs = check();
    std::cout << "inxi recommended tools check:\n";
    std::cout << "---------------------------------------------------------\n";
    for (const auto& r : recs) {
        std::cout << std::left << std::setw(15) << r.name
                  << " [" << (r.installed ? "INSTALLED" : "MISSING  ") << "] "
                  << r.purpose << "\n";
    }
    std::cout << "---------------------------------------------------------\n";
}

}  // namespace inxi
