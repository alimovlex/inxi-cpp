// system_item.cpp
#include "items/system_item.h"
#include "utils/file_utils.h"
#include "utils/process_utils.h"
#include "utils/string_utils.h"
#include <cstdlib>
#include <sstream>
#include <sys/utsname.h>
#include <unistd.h>

namespace inxi {

void SystemItem::kernel_data(SystemData& d) {
    struct utsname un;
    if (uname(&un) == 0) {
        d.hostname = un.nodename;
        d.kernel = un.release;
        d.kernel_bits = (sizeof(void*) == 8) ? "64" : "32";
    }
}

void SystemItem::distro_data(SystemData& d) {
    auto lines = utils::read_file("/etc/os-release");
    if (lines.empty()) {
        lines = utils::read_file("/usr/lib/os-release");
    }
    for (const auto& line : lines) {
        if (line.rfind("PRETTY_NAME=", 0) == 0) {
            std::string val = line.substr(12);
            if (val.size() >= 2 && val.front() == '"' && val.back() == '"') {
                val = val.substr(1, val.size() - 2);
            }
            d.distro = val;
        } else if (line.rfind("ID=", 0) == 0) {
            d.distro_id = line.substr(3);
        } else if (line.rfind("VERSION_ID=", 0) == 0) {
            d.distro_ver = line.substr(11);
        }
    }
    if (d.distro.empty()) {
        std::string issue = utils::read_file_str("/etc/issue");
        if (!issue.empty()) {
            d.distro = utils::trim(utils::split_lines(issue)[0]);
        } else {
            d.distro = "Linux";
        }
    }
}

void SystemItem::de_data(SystemData& d) {
    const char* xdg_de = std::getenv("XDG_CURRENT_DESKTOP");
    const char* dsession = std::getenv("DESKTOP_SESSION");
    if (xdg_de && *xdg_de) {
        d.desktop = xdg_de;
    } else if (dsession && *dsession) {
        d.desktop = dsession;
    } else {
        // Probe common desktop processes
        if (!utils::run_capture("pgrep -x xfce4-session").empty()) {
            d.desktop = "Xfce";
        } else if (!utils::run_capture("pgrep -x gnome-shell").empty()) {
            d.desktop = "GNOME";
        } else if (!utils::run_capture("pgrep -x plasmashell").empty()) {
            d.desktop = "KDE Plasma";
        } else if (!utils::run_capture("pgrep -x mate-session").empty()) {
            d.desktop = "MATE";
        }
    }

    if (d.desktop.find("XFCE") != std::string::npos || d.desktop == "Xfce") {
        d.desktop = "Xfce";
        std::string v = utils::program_version_cmd("xfce4-session");
        if (!v.empty()) d.de_id = v;
    } else if (d.desktop.find("GNOME") != std::string::npos) {
        d.desktop = "GNOME";
        std::string v = utils::program_version_cmd("gnome-shell");
        if (!v.empty()) d.de_id = v;
    } else if (d.desktop.find("KDE") != std::string::npos) {
        d.desktop = "KDE Plasma";
        std::string v = utils::program_version_cmd("plasmashell");
        if (!v.empty()) d.de_id = v;
    }
}

void SystemItem::shell_data(SystemData& d) {
    const char* s = std::getenv("SHELL");
    d.shell = s ? s : "sh";
}

void SystemItem::wm_data(SystemData& d) {}
void SystemItem::theme_data(SystemData& d) {}

std::string SystemItem::uptime_data() {
    std::string uptime_str = utils::read_file_str("/proc/uptime");
    if (uptime_str.empty()) return "";
    double total_secs = std::stod(utils::get_piece(uptime_str, 0));
    long secs = static_cast<long>(total_secs);
    long days = secs / 86400;
    long hours = (secs % 86400) / 3600;
    long mins = (secs % 3600) / 60;
    std::ostringstream ss;
    if (days > 0) ss << days << "d ";
    if (hours > 0 || days > 0) ss << hours << "h ";
    ss << mins << "m";
    return ss.str();
}

SystemData SystemItem::system_data() {
    SystemData d;
    kernel_data(d);
    distro_data(d);
    de_data(d);
    shell_data(d);
    d.uptime = uptime_data();
    return d;
}

std::vector<OutputRow> SystemItem::short_output(const SystemData& d) {
    return full_output(d);
}

std::vector<OutputRow> SystemItem::full_output(const SystemData& d) {
    std::vector<OutputRow> rows;
    std::vector<Field> f1;
    if (!state_.filter && !d.hostname.empty()) {
        f1.push_back(field("Host", d.hostname));
    }
    f1.push_back(field("Kernel", d.kernel));
    f1.push_back(field("arch", state_.cpu_arch));
    f1.push_back(field("bits", std::to_string(state_.bits_sys)));

    // -x: compiler used to build the kernel
    if (state_.extra >= 1) {
        std::string gcc_ver = utils::program_version_cmd("gcc");
        if (!gcc_ver.empty()) {
            f1.push_back(field("compiler", "gcc"));
            f1.push_back(field("v", gcc_ver));
        }
    }

    // -xxx: clocksource
    if (state_.extra >= 3) {
        std::string cs = utils::trim(utils::read_file_str(
            "/sys/devices/system/clocksource/clocksource0/current_clocksource"));
        if (!cs.empty()) f1.push_back(field("clocksource", cs));
        // available clocksources
        std::string avail = utils::trim(utils::read_file_str(
            "/sys/devices/system/clocksource/clocksource0/available_clocksource"));
        if (!avail.empty()) {
            // comma-join
            std::string avail_fmt;
            for (auto& ch : avail) if (ch == ' ') ch = ',';
            avail_fmt = avail;
            f1.push_back(field("avail", avail_fmt));
        }
    }

    rows.push_back(make_row("System", f1));

    std::vector<Field> f2;
    if (!d.desktop.empty()) {
        f2.push_back(field("Desktop", d.desktop));
        if (!d.de_id.empty()) {
            f2.push_back(field("v", d.de_id));
        }
    }
    f2.push_back(field("Distro", d.distro));
    rows.push_back(make_row("", f2, true));

    return rows;
}

std::vector<OutputRow> SystemItem::get() {
    auto data = system_data();
    return full_output(data);
}

}  // namespace inxi
