#include "graphics_module.h"
#include "common.h"

#include <cstdlib>
#include <vector>

namespace mod {
using namespace util;

namespace {

std::vector<std::string> findGraphicsDevices() {
    std::vector<std::string> devices;
    if (!commandExists("lspci")) return devices;
    std::string out = runCommand("lspci");
    static const std::vector<std::string> markers = {
        "VGA compatible controller: ", "3D controller: ", "Display controller: "
    };
    for (const auto& line : split(out, '\n')) {
        for (const auto& marker : markers) {
            auto pos = line.find(marker);
            if (pos != std::string::npos) {
                devices.push_back(trim(line.substr(pos + marker.size())));
                break;
            }
        }
    }
    return devices;
}

std::string detectDisplayServer() {
    if (const char* wl = std::getenv("WAYLAND_DISPLAY"); wl && *wl) return "Wayland";
    if (const char* d = std::getenv("DISPLAY"); d && *d) return "X11";
    return "N/A (headless/console)";
}

// Best-effort: ask xrandr which mode is currently active ("*" flag).
std::string detectResolutionX11() {
    if (!commandExists("xrandr")) return "";
    std::string out = runCommand("xrandr --current");
    for (const auto& line : split(out, '\n')) {
        auto connPos = line.find(" connected ");
        if (connPos == std::string::npos) continue;
        std::string rest = trim(line.substr(connPos + std::string(" connected ").size()));
        auto spacePos = rest.find(' ');
        std::string token = spacePos == std::string::npos ? rest : rest.substr(0, spacePos);
        auto plusPos = token.find('+');
        if (plusPos != std::string::npos) token = token.substr(0, plusPos);
        if (token.find('x') != std::string::npos) return token;
    }
    return "";
}

} // namespace

void printGraphicsSection(std::ostream& os, bool color) {
    SectionPrinter p(os, color);
    p.header("Graphics");

    auto devices = findGraphicsDevices();
    if (devices.empty()) {
        p.beginLine();
        if (!commandExists("lspci")) {
            p.field("Message", "Required tool lspci not installed - can't list GPU device(s).");
        } else {
            p.field("Message", "No GPU device found via lspci.");
        }
        p.endLine();
    } else {
        int idx = 1;
        for (const auto& dev : devices) {
            p.beginLine();
            p.field("Device-" + std::to_string(idx++), dev);
            p.endLine();
        }
    }

    std::string server = detectDisplayServer();
    p.beginLine();
    p.field("Display", server);
    if (server == "X11") {
        std::string res = detectResolutionX11();
        p.field("resolution", res.empty() ? "N/A" : res);
    }
    p.endLine();
}

} // namespace mod
