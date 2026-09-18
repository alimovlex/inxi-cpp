// process_utils.cpp
#include "utils/process_utils.h"
#include "utils/string_utils.h"
#include <array>
#include <cstdlib>
#include <filesystem>
#include <regex>
#include <sstream>
#include <unistd.h>

namespace inxi::utils {

std::string check_program(const std::string& name, const std::vector<std::string>& extra_paths) {
    if (name.empty()) return "";
    // If it's already an absolute or relative path
    if (name.find('/') != std::string::npos) {
        if (access(name.c_str(), X_OK) == 0) return name;
        return "";
    }

    std::vector<std::string> search_dirs = extra_paths;
    const char* path_env = std::getenv("PATH");
    if (path_env) {
        auto env_dirs = split(path_env, ':');
        search_dirs.insert(search_dirs.end(), env_dirs.begin(), env_dirs.end());
    }
    // Standard system paths that might not be in a user's PATH
    std::vector<std::string> sys_dirs = {
        "/usr/local/sbin", "/usr/local/bin", "/usr/sbin", "/usr/bin", "/sbin", "/bin"
    };
    search_dirs.insert(search_dirs.end(), sys_dirs.begin(), sys_dirs.end());

    for (const auto& dir : search_dirs) {
        if (dir.empty()) continue;
        std::string full_path = dir + "/" + name;
        if (access(full_path.c_str(), X_OK) == 0) {
            return full_path;
        }
    }
    return "";
}

std::vector<std::string> run_capture(const std::string& cmd) {
    std::vector<std::string> lines;
    if (cmd.empty()) return lines;

    std::string safe_cmd = cmd + " 2>/dev/null";
    FILE* pipe = popen(safe_cmd.c_str(), "r");
    if (!pipe) return lines;

    std::array<char, 512> buffer;
    std::string output;
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        output += buffer.data();
    }
    pclose(pipe);

    return split_lines(output);
}

std::vector<std::string> run_capture_merged(const std::string& cmd) {
    std::vector<std::string> lines;
    if (cmd.empty()) return lines;

    std::string safe_cmd = cmd + " 2>&1";
    FILE* pipe = popen(safe_cmd.c_str(), "r");
    if (!pipe) return lines;

    std::array<char, 512> buffer;
    std::string output;
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        output += buffer.data();
    }
    pclose(pipe);

    return split_lines(output);
}

ProgramInfo program_data(const std::string& prog, const std::vector<std::string>& paths) {
    ProgramInfo info;
    info.name = prog;
    info.path = check_program(prog, paths);
    if (!info.path.empty()) {
        info.version = program_version_cmd(prog);
    }
    return info;
}

std::string program_version_cmd(const std::string& prog, const std::string& arg) {
    std::string path = check_program(prog);
    if (path.empty()) return "";

    auto lines = run_capture(path + " " + arg);
    if (lines.empty()) return "";

    // Search lines for version pattern
    std::regex ver_regex(R"((\d+(\.\d+)+))");
    for (const auto& line : lines) {
        std::smatch m;
        if (std::regex_search(line, m, ver_regex)) {
            return m[1].str();
        }
    }
    return "";
}

std::string program_version(const std::string& prog, const std::vector<std::string>& args, const std::string& version_key) {
    std::string path = check_program(prog);
    if (path.empty()) return "";

    std::string cmd = path + " " + join(args, " ");
    auto lines = run_capture(cmd);
    for (const auto& line : lines) {
        if (!version_key.empty() && line.find(version_key) == std::string::npos) {
            continue;
        }
        std::regex ver_regex(R"((\d+(\.\d+)+))");
        std::smatch m;
        if (std::regex_search(line, m, ver_regex)) {
            return m[1].str();
        }
    }
    return "";
}

std::string program_version_pkg(const std::string& prog) {
    (void)prog;
    return "";
}

std::string awk_field(const std::string& line, int field_n) {
    if (field_n <= 0) return "";
    std::istringstream iss(line);
    std::string token;
    int current = 1;
    while (iss >> token) {
        if (current == field_n) return token;
        current++;
    }
    return "";
}

std::vector<std::string> get_package_managers(const std::vector<std::string>& paths) {
    static const std::vector<std::string> pms = {
        "xbps-install", "pacman", "apt-get", "dpkg", "dnf", "yum", "zypper",
        "emerge", "apk", "flatpak", "snap", "nix-env", "guix", "brew"
    };
    std::vector<std::string> found;
    for (const auto& pm : pms) {
        if (!check_program(pm, paths).empty()) {
            found.push_back(pm);
        }
    }
    return found;
}

}  // namespace inxi::utils
