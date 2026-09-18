#include "network_module.h"
#include "common.h"

#include <filesystem>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

namespace mod {
using namespace util;

namespace {

std::string maskMac(const std::string& mac) {
    auto parts = split(mac, ':');
    if (parts.size() < 6) return mac;
    return parts[0] + ":" + parts[1] + ":" + parts[2] + ":xx:xx:xx";
}

std::string detectDriver(const std::string& iface) {
    std::error_code ec;
    fs::path link = "/sys/class/net/" + iface + "/device/driver";
    fs::path target = fs::read_symlink(link, ec);
    if (ec) return "";
    return target.filename().string();
}

} // namespace

void printNetworkSection(std::ostream& os, bool color, bool filterMac) {
    SectionPrinter p(os, color);
    p.header("Network");

    std::vector<std::string> ifaces;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator("/sys/class/net", ec)) {
        std::string name = entry.path().filename().string();
        if (name == "lo") continue;
        ifaces.push_back(name);
    }
    std::sort(ifaces.begin(), ifaces.end());

    if (ifaces.empty()) {
        p.beginLine();
        p.field("Message", "No network interfaces found under /sys/class/net.");
        p.endLine();
        return;
    }

    int idx = 1;
    for (const auto& iface : ifaces) {
        std::string base = "/sys/class/net/" + iface;
        std::string state = readFirstLine(base + "/operstate");
        std::string mac = readFirstLine(base + "/address");
        std::string speed = readFirstLine(base + "/speed");
        std::string driver = detectDriver(iface);

        p.beginLine();
        p.field("Device-" + std::to_string(idx++), iface);
        p.field("driver", driver.empty() ? "N/A (virtual?)" : driver);
        p.field("state", state.empty() ? "unknown" : state);
        if (!speed.empty() && speed != "-1") p.field("speed", speed + " Mbps");
        if (!mac.empty()) p.field("mac", filterMac ? maskMac(mac) : mac);
        p.endLine();
    }
}

} // namespace mod
