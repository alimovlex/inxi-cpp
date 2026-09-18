#include "system_module.h"
#include "common.h"

#include <sys/utsname.h>
#include <unistd.h>
#include <cstdlib>
#include <array>

namespace mod {
using namespace util;

std::string getKernelRelease() {
    struct utsname u{};
    if (uname(&u) != 0) return "N/A";
    return std::string(u.release);
}

std::string getKernelArch() {
    struct utsname u{};
    if (uname(&u) != 0) return "N/A";
    return std::string(u.machine);
}

std::string getHostname() {
    char buf[256];
    if (gethostname(buf, sizeof(buf)) != 0) return "N/A";
    return std::string(buf);
}

std::string getDistroPretty() {
    for (const auto& line : readLines("/etc/os-release")) {
        if (startsWith(line, "PRETTY_NAME=")) {
            std::string v = line.substr(std::string("PRETTY_NAME=").size());
            if (v.size() >= 2 && v.front() == '"' && v.back() == '"') {
                v = v.substr(1, v.size() - 2);
            }
            return v;
        }
    }
    return "N/A";
}

static std::string detectDesktop() {
    if (const char* xdg = std::getenv("XDG_CURRENT_DESKTOP"); xdg && *xdg) return xdg;
    if (const char* ds = std::getenv("DESKTOP_SESSION"); ds && *ds) return ds;
    if (const char* wl = std::getenv("WAYLAND_DISPLAY"); wl && *wl) return "Wayland session (compositor unknown)";
    if (const char* d = std::getenv("DISPLAY"); d && *d) return "X11 session (DE unknown)";
    return "N/A (console)";
}

void printSystemSection(std::ostream& os, bool color) {
    SectionPrinter p(os, color);
    p.header("System");

    p.beginLine();
    p.field("Host", getHostname());
    p.field("Kernel", getKernelRelease());
    p.field("arch", getKernelArch());
    p.endLine();

    p.beginLine();
    p.field("Desktop", detectDesktop());
    p.endLine();

    p.beginLine();
    p.field("Distro", getDistroPretty());
    p.endLine();
}

// ---- Machine (-M) ---------------------------------------------------------
// inxi reads the same /sys/class/dmi/id files (falling back to dmidecode,
// as root, on older kernels). This port reads /sys only; it does not shell
// out to dmidecode.

static std::string dmi(const std::string& file) {
    return readFirstLine("/sys/class/dmi/id/" + file);
}

static std::string chassisTypeName(const std::string& code) {
    static const std::array<const char*, 37> table = {
        "N/A",              // 0 (unused)
        "Other", "Unknown", "Desktop", "Low Profile Desktop", "Pizza Box",
        "Mini Tower", "Tower", "Portable", "Laptop", "Notebook",
        "Hand Held", "Docking Station", "All In One", "Sub Notebook", "Space-saving",
        "Lunch Box", "Main Server Chassis", "Expansion Chassis", "Sub Chassis", "Bus Expansion Chassis",
        "Peripheral Chassis", "RAID Chassis", "Rack Mount Chassis", "Sealed-case PC", "Multi-system",
        "CompactPCI", "AdvancedTCA", "Blade", "Blade Enclosure", "Tablet",
        "Convertible", "Detachable", "IoT Gateway", "Embedded PC", "Mini PC",
        "Stick PC"
    };
    try {
        size_t idx = std::stoul(code);
        if (idx > 0 && idx < table.size()) return table[idx];
    } catch (...) {}
    return "N/A";
}

void printMachineSection(std::ostream& os, bool color) {
    SectionPrinter p(os, color);
    p.header("Machine");

    std::string sysVendor = dmi("sys_vendor");
    std::string productName = dmi("product_name");
    std::string boardName = dmi("board_name");
    std::string biosVendor = dmi("bios_vendor");
    std::string biosVersion = dmi("bios_version");
    std::string biosDate = dmi("bios_date");
    std::string chassis = dmi("chassis_type");

    if (sysVendor.empty() && productName.empty() && boardName.empty()) {
        p.beginLine();
        p.field("Message", "No DMI data available under /sys/class/dmi/id "
                            "(common in VMs/containers, or needs root on some kernels).");
        p.endLine();
        return;
    }

    p.beginLine();
    if (!chassis.empty()) p.field("Type", chassisTypeName(chassis));
    p.field("System", sysVendor.empty() ? "N/A" : sysVendor);
    p.field("product", productName.empty() ? "N/A" : productName);
    p.endLine();

    if (!boardName.empty()) {
        p.beginLine();
        p.field("Mobo", dmi("board_vendor").empty() ? "N/A" : dmi("board_vendor"));
        p.field("model", boardName);
        p.endLine();
    }

    if (!biosVendor.empty() || !biosVersion.empty()) {
        p.beginLine();
        p.field("BIOS", biosVendor.empty() ? "N/A" : biosVendor);
        p.field("v", biosVersion.empty() ? "N/A" : biosVersion);
        if (!biosDate.empty()) p.field("date", biosDate);
        p.endLine();
    }
}

} // namespace mod
