// initialize.cpp
#include "initialize.h"
#include "utils/file_utils.h"
#include "utils/process_utils.h"
#include "utils/string_utils.h"
#include <algorithm>
#include <cstdlib>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <unistd.h>

namespace inxi {

void initialize(GlobalState& state) {
    set_os(state);
    set_path(state);
    set_user_paths(state);
    set_basics(state);
    set_system_files(state);
    set_display_size(state);
    set_sep(state);
    set_sudo(state);
    set_xorg_log(state);
}

void set_os(GlobalState& state) {
    struct utsname un;
    if (uname(&un) == 0) {
        state.os = utils::to_lower(un.sysname);
        state.cpu_arch = un.machine;
        state.uname_vals = {un.sysname, un.nodename, un.release, un.version, un.machine};
    } else {
        state.os = "linux";
        state.cpu_arch = "x86_64";
    }
    state.bits_sys = (sizeof(void*) == 8) ? 64 : 32;

    std::string arch = state.cpu_arch;
    if (arch.rfind("arm", 0) == 0 || arch.rfind("aarch", 0) == 0) {
        state.risc.arm = true;
        state.risc.id = "arm";
    } else if (arch.rfind("mips", 0) == 0) {
        state.risc.mips = true;
        state.risc.id = "mips";
    } else if (arch.rfind("ppc", 0) == 0) {
        state.risc.ppc = true;
        state.risc.id = "ppc";
    } else if (arch.rfind("riscv", 0) == 0) {
        state.risc.riscv = true;
        state.risc.id = "riscv";
    }
}

void set_path(GlobalState& state) {
    const char* path_env = std::getenv("PATH");
    if (path_env) {
        state.paths = utils::split(path_env, ':');
    }
    std::vector<std::string> defaults = {
        "/usr/local/sbin", "/usr/local/bin", "/usr/sbin", "/usr/bin", "/sbin", "/bin"
    };
    for (const auto& p : defaults) {
        if (std::find(state.paths.begin(), state.paths.end(), p) == state.paths.end()) {
            state.paths.push_back(p);
        }
    }
}

void set_user_paths(GlobalState& state) {
    const char* home = std::getenv("HOME");
    std::string home_str = home ? home : "";
    const char* xdg_config = std::getenv("XDG_CONFIG_HOME");
    if (xdg_config && *xdg_config) {
        state.user_config_dir = std::string(xdg_config) + "/inxi";
    } else if (!home_str.empty()) {
        state.user_config_dir = home_str + "/.config/inxi";
    } else {
        state.user_config_dir = "/etc/inxi";
    }
    state.user_config_file = state.user_config_dir + "/inxi.conf";

    const char* xdg_data = std::getenv("XDG_DATA_HOME");
    if (xdg_data && *xdg_data) {
        state.user_data_dir = std::string(xdg_data) + "/inxi";
    } else if (!home_str.empty()) {
        state.user_data_dir = home_str + "/.local/share/inxi";
    }
}

void set_basics(GlobalState& state) {
    state.b_root = (geteuid() == 0);
    const char* user = std::getenv("USER");
    if (user) {
        state.client.whoami = user;
    }
    const char* lang = std::getenv("LANG");
    if (lang) {
        state.language = lang;
    }
}

void set_system_files(GlobalState& state) {
    state.system_files.proc_cpuinfo = "/proc/cpuinfo";
    state.system_files.proc_meminfo = "/proc/meminfo";
    state.system_files.proc_mounts = "/proc/mounts";
    state.system_files.proc_partitions = "/proc/partitions";
    state.system_files.proc_version = "/proc/version";
    state.system_files.proc_cmdline = "/proc/cmdline";
    state.system_files.proc_mdstat = "/proc/mdstat";
    state.system_files.proc_modules = "/proc/modules";
    state.system_files.proc_scsi = "/proc/scsi/scsi";
    state.system_files.asound_cards = "/proc/asound/cards";
    state.system_files.asound_modules = "/proc/asound/modules";
    state.system_files.asound_version = "/proc/asound/version";
}

void set_display_size(GlobalState& state) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        state.size.term_cols = ws.ws_col;
        state.size.term_lines = ws.ws_row;
    } else {
        const char* cols = std::getenv("COLUMNS");
        if (cols) {
            state.size.term_cols = std::max(40, std::atoi(cols));
        } else {
            state.size.term_cols = 80;
        }
    }
    state.size.max_wrap = std::max(80, state.size.term_cols);
}

void set_sep(GlobalState& state) {
    state.sep.s1 = ":";
    state.sep.s2 = ":";
}

void set_sudo(GlobalState& state) {
    if (!utils::check_program("doas").empty() && !state.force.no_doas) {
        state.sudoas = "doas";
    } else if (!utils::check_program("sudo").empty() && !state.force.no_sudo) {
        state.sudoas = "sudo";
    }
}

void set_xorg_log(GlobalState& state) {
    const char* home = std::getenv("HOME");
    std::string user_log = home ? std::string(home) + "/.local/share/xorg/Xorg.0.log" : "";
    if (!user_log.empty() && access(user_log.c_str(), R_OK) == 0) {
        state.system_files.xorg_log = user_log;
    } else if (access("/var/log/Xorg.0.log", R_OK) == 0) {
        state.system_files.xorg_log = "/var/log/Xorg.0.log";
    }
}

void cleanup(GlobalState& state) {
    (void)state;
    // Flush any open debug file handles or temporary artifacts
}

}  // namespace inxi
