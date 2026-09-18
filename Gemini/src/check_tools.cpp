// check_tools.cpp
#include "check_tools.h"
#include "utils/process_utils.h"

namespace inxi {

CheckTools::CheckTools(GlobalState& state) : state_(state) {}

void CheckTools::probe() {
    build_command_table();
}

void CheckTools::build_command_table() {
    static const std::vector<std::string> tools = {
        "lspci", "lsblk", "dmidecode", "ip", "ifconfig", "smartctl",
        "sensors", "bluetoothctl", "hciconfig", "df", "free", "uptime",
        "glxinfo", "xrandr", "upower", "rfkill", "ps"
    };

    for (const auto& t : tools) {
        std::string path = utils::check_program(t, state_.paths);
        Alert a;
        if (!path.empty()) {
            a.action = "use";
            a.path = path;
        } else {
            a.action = "missing";
            a.message = t + " not found";
        }
        state_.alerts[t] = a;
    }
}

std::string CheckTools::probe_dmidecode(const std::vector<std::string>& output) {
    for (const auto& line : output) {
        if (line.find("Permission denied") != std::string::npos ||
            line.find("No SMBIOS nor DMI entry point found") != std::string::npos) {
            return "permissions";
        }
    }
    return "ok";
}

void CheckTools::fake_bsd_tools() {}

}  // namespace inxi
