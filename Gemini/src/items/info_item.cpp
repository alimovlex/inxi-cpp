// info_item.cpp
#include "items/info_item.h"
#include "utils/file_utils.h"
#include "utils/math_utils.h"
#include "utils/process_utils.h"
#include "utils/string_utils.h"
#include <cstdlib>
#include <dirent.h>
#include <iomanip>
#include <pwd.h>
#include <sstream>
#include <sys/sysinfo.h>
#include <unistd.h>

namespace inxi {

// Helper: get the basename of SHELL env, and its version string
static std::string shell_version(const std::string& shell_name) {
    // Most shells support --version or -c 'echo $VERSION'
    static const std::pair<std::string, std::string> cmds[] = {
        {"bash",  "bash --version 2>/dev/null | head -1"},
        {"zsh",   "zsh --version 2>/dev/null"},
        {"fish",  "fish --version 2>/dev/null"},
        {"dash",  ""},
        {"ksh",   "ksh --version 2>/dev/null"},
        {"tcsh",  "tcsh --version 2>/dev/null"},
        {"mksh",  "mksh -c 'echo $KSH_VERSION' 2>/dev/null"},
    };
    for (const auto& [name, cmd] : cmds) {
        if (shell_name == name && !cmd.empty()) {
            auto out = utils::run_capture(cmd);
            if (!out.empty()) {
                // Extract first version-like token e.g. "5.3.0"
                std::string line = out[0];
                std::string ver;
                bool in_digit = false;
                for (char c : line) {
                    if (std::isdigit(c) || (in_digit && (c == '.' || c == '-'))) {
                        ver += c;
                        in_digit = true;
                    } else if (in_digit) {
                        // stop at first non-version char after starting
                        if (ver.size() > 1) break;
                        ver.clear();
                        in_digit = false;
                    }
                }
                // must look like a real version (at least X.Y)
                if (ver.find('.') != std::string::npos) return ver;
            }
            break;
        }
    }
    return {};
}

// Helper: detect running terminal / multiplexer name
static std::string detect_running_in() {
    // Check standard env vars set by terminal emulators
    const char* term_prog = std::getenv("TERM_PROGRAM");
    if (term_prog && *term_prog) {
        std::string t(term_prog);
        if (t == "iTerm.app") return "iTerm2";
        if (t == "vscode") return "vscode";
        if (t == "tmux") return "tmux";
        return t;
    }
    const char* term = std::getenv("TERM");
    if (term) {
        std::string t(term);
        if (t.rfind("tmux", 0) == 0) return "tmux";
        if (t == "screen") return "screen";
    }
    if (std::getenv("TMUX")) return "tmux";
    if (std::getenv("STY"))  return "screen";
    const char* lc = std::getenv("LC_TERMINAL");
    if (lc && *lc) return std::string(lc);
    return {};
}

// Helper: read power suspend / hibernate states
static void read_power_info(std::string& states, std::string& suspend,
                             std::string& hibernate_avail) {
    states   = utils::trim(utils::read_file_str("/sys/power/state"));
    // mem_sleep contains the available suspend modes; [deep] = current
    std::string mem_sleep = utils::trim(utils::read_file_str("/sys/power/mem_sleep"));
    if (!mem_sleep.empty()) {
        // extract bracketed item as current
        size_t lb = mem_sleep.find('['), rb = mem_sleep.find(']');
        if (lb != std::string::npos && rb != std::string::npos && rb > lb) {
            suspend = mem_sleep.substr(lb + 1, rb - lb - 1);
        }
    }
    // /sys/power/disk shows hibernate options
    std::string disk_str = utils::trim(utils::read_file_str("/sys/power/disk"));
    if (!disk_str.empty()) {
        // strip brackets → available options
        std::string avail;
        for (char c : disk_str) {
            if (c != '[' && c != ']') avail += c;
        }
        // collapse multiple spaces
        hibernate_avail = utils::trim(avail);
    }
}

std::vector<OutputRow> GeneralInfoItem::get() {
    std::vector<OutputRow> rows;

    // 1. Processes count (count numeric dirs in /proc)
    int procs = 0;
    DIR* proc_dir = opendir("/proc");
    if (proc_dir) {
        struct dirent* entry;
        while ((entry = readdir(proc_dir)) != nullptr) {
            if (entry->d_type == DT_DIR && utils::is_int(entry->d_name)) {
                procs++;
            }
        }
        closedir(proc_dir);
    }
    if (procs == 0) {
        struct sysinfo si;
        if (sysinfo(&si) == 0) procs = si.procs;
    }

    // 2. Uptime
    std::string uptime_str = utils::read_file_str("/proc/uptime");
    std::string uptime_fmt;
    if (!uptime_str.empty()) {
        double total_secs = std::stod(utils::get_piece(uptime_str, 0));
        long secs = static_cast<long>(total_secs);
        long days = secs / 86400;
        long hours = (secs % 86400) / 3600;
        long mins = (secs % 3600) / 60;
        std::ostringstream up_ss;
        if (days > 0) up_ss << days << "d ";
        if (hours > 0 || days > 0) up_ss << hours << "h ";
        up_ss << mins << "m";
        uptime_fmt = up_ss.str();
    }

    // 3. Memory
    auto mem_lines = utils::read_file("/proc/meminfo");
    uint64_t mem_total = 0, mem_avail = 0;
    for (const auto& line : mem_lines) {
        if (line.rfind("MemTotal:", 0) == 0) {
            mem_total = std::stoull(utils::awk_field(line, 2)) * 1024ULL;
        } else if (line.rfind("MemAvailable:", 0) == 0) {
            mem_avail = std::stoull(utils::awk_field(line, 2)) * 1024ULL;
        }
    }

    uint64_t phys_total = 0;
    std::string bsize_hex = utils::trim(utils::read_file_str("/sys/devices/system/memory/block_size_bytes"));
    if (!bsize_hex.empty()) {
        uint64_t bsize = utils::convert_hex(bsize_hex);
        auto online_files = utils::glob_files("/sys/devices/system/memory/memory*/online");
        int online_count = 0;
        for (const auto& of : online_files) {
            if (utils::trim(utils::read_file_str(of)) == "1") {
                online_count++;
            }
        }
        if (online_count > 0 && bsize > 0) {
            phys_total = static_cast<uint64_t>(online_count) * bsize;
        }
    }

    uint64_t mem_used = (mem_total >= mem_avail) ? (mem_total - mem_avail) : 0;
    std::string used_h = utils::format_bytes(mem_used, 2);
    std::string total_h = utils::format_bytes(mem_total, 2);
    std::string avail_h = utils::format_bytes(mem_avail, 2);
    std::string pct = utils::format_percent(static_cast<double>(mem_used), static_cast<double>(mem_total));

    // 4. Shell — running shell + default shell
    std::string running_shell_name;
    const char* shell_env = std::getenv("SHELL");
    if (shell_env) {
        std::string s(shell_env);
        size_t slash = s.rfind('/');
        running_shell_name = (slash != std::string::npos) ? s.substr(slash + 1) : s;
    }
    if (running_shell_name.empty()) running_shell_name = state_.client.name;

    // Detect actual running shell from parent process name
    std::string running_shell_ver = shell_version(running_shell_name);

    // Default shell from /etc/passwd
    std::string default_shell_name;
    std::string default_shell_ver;
    uid_t uid = getuid();
    struct passwd* pw = getpwuid(uid);
    if (pw && pw->pw_shell) {
        std::string def(pw->pw_shell);
        size_t sl = def.rfind('/');
        default_shell_name = (sl != std::string::npos) ? def.substr(sl + 1) : def;
        if (default_shell_name != running_shell_name) {
            default_shell_ver = shell_version(default_shell_name);
        }
    }

    std::string running_in = detect_running_in();

    if (state_.extra >= 1) {
        // Multi-row layout matching reference inxi
        std::vector<Field> m_fields;
        if (phys_total > 0) {
            std::string phys_h = utils::format_bytes(phys_total, 0);
            m_fields.push_back(field("Memory", "total: " + phys_h + " available: " + avail_h + " used: " + used_h + " (" + pct + ")"));
        } else {
            m_fields.push_back(field("Memory", "total: " + total_h + " available: " + avail_h + " used: " + used_h + " (" + pct + ")"));
        }
        rows.push_back(make_row("Info", m_fields));

        // Processes + Uptime + Power
        std::vector<Field> p_fields;
        p_fields.push_back(field("Processes", std::to_string(procs)));
        if (!uptime_fmt.empty()) {
            p_fields.push_back(field("Uptime", uptime_fmt));
        }

        // Power states (extra >= 1)
        {
            std::string states, suspend_mode, hibernate_avail;
            read_power_info(states, suspend_mode, hibernate_avail);
            if (!states.empty()) {
                // replace spaces with commas for display
                std::string states_fmt;
                for (char c : states) states_fmt += (c == ' ' ? ',' : c);
                p_fields.push_back(field("states", states_fmt));
            }
            if (!suspend_mode.empty()) {
                p_fields.push_back(field("suspend", suspend_mode));
            }
            if (!hibernate_avail.empty() && state_.extra >= 2) {
                // show avail modes
                std::string ha_fmt;
                for (char c : hibernate_avail) ha_fmt += (c == ' ' ? ',' : c);
                p_fields.push_back(field("avail", ha_fmt));
            }
        }

        rows.push_back(make_row("", p_fields, true));

        // Packages + Compilers + Shell
        std::vector<Field> pkg_fields;

        // Check package manager
        if (!utils::check_program("xbps-query").empty()) {
            auto out = utils::run_capture("xbps-query -l 2>/dev/null | wc -l");
            if (!out.empty()) {
                std::string pkg_val = "pm: xbps pkgs: " + utils::trim(out[0]);
                // count shared libs (lines with lib in name)
                auto libs_out = utils::run_capture(
                    "xbps-query -l 2>/dev/null | grep -c ' lib' || true");
                if (!libs_out.empty() && utils::is_int(utils::trim(libs_out[0]))) {
                    pkg_val += " libs: " + utils::trim(libs_out[0]);
                }
                pkg_fields.push_back(field("Packages", pkg_val));
            }
        } else if (!utils::check_program("pacman").empty()) {
            auto out = utils::run_capture("pacman -Qq 2>/dev/null | wc -l");
            if (!out.empty()) {
                pkg_fields.push_back(field("Packages", "pm: pacman pkgs: " + utils::trim(out[0])));
            }
        } else if (!utils::check_program("dpkg-query").empty()) {
            auto out = utils::run_capture("dpkg-query -l 2>/dev/null | grep -c '^ii'");
            if (!out.empty()) {
                pkg_fields.push_back(field("Packages", "pm: dpkg pkgs: " + utils::trim(out[0])));
            }
        }

        // Compilers
        std::string clang_ver = utils::program_version_cmd("clang");
        std::string gcc_ver   = utils::program_version_cmd("gcc");
        if (!clang_ver.empty() || !gcc_ver.empty()) {
            std::string comp;
            if (!clang_ver.empty()) comp += "clang: " + clang_ver;
            if (!gcc_ver.empty())   comp += std::string(comp.empty() ? "" : " ") + "gcc: " + gcc_ver;
            pkg_fields.push_back(field("Compilers", comp));
        }

        // Shell: running shell [v: ver] [default: X [v: ver]] [running-in: X]
        {
            std::string shell_str = running_shell_name;
            if (!running_shell_ver.empty()) shell_str += " v: " + running_shell_ver;
            if (!default_shell_name.empty() && default_shell_name != running_shell_name) {
                shell_str += " default: " + default_shell_name;
                if (!default_shell_ver.empty()) shell_str += " v: " + default_shell_ver;
            }
            if (!running_in.empty()) shell_str += " running-in: " + running_in;
            pkg_fields.push_back(field("Shell", shell_str));
        }

        pkg_fields.push_back(field(state_.self_name, state_.self_version));
        rows.push_back(make_row("", pkg_fields, true));
    } else {
        std::vector<Field> fields;
        fields.push_back(field("Processes", std::to_string(procs)));
        if (!uptime_fmt.empty()) {
            fields.push_back(field("Uptime", uptime_fmt));
        }
        if (mem_total > 0) {
            fields.push_back(field("Memory", "total: " + total_h + " available: " + avail_h
                                   + " used: " + used_h + " (" + pct + ")"));
        }
        // Shell
        {
            std::string shell_str = running_shell_name;
            if (!running_shell_ver.empty()) shell_str += " v: " + running_shell_ver;
            fields.push_back(field("Shell", shell_str));
        }
        fields.push_back(field(state_.self_name, state_.self_version));
        rows.push_back(make_row("Info", fields));
    }

    return rows;
}

}  // namespace inxi
