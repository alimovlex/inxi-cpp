// file_utils.cpp
#include "utils/file_utils.h"
#include "utils/string_utils.h"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <glob.h>
#include <sstream>
#include <utime.h>

namespace inxi::utils {

std::vector<std::string> read_file(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream file(path);
    if (!file.is_open()) return lines;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
    }
    return lines;
}

std::string read_file_str(const std::string& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) return "";
    std::ostringstream ss;
    ss << file.rdbuf();
    return trim(ss.str());
}

bool write_file(const std::string& path, const std::vector<std::string>& lines) {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    for (const auto& line : lines) {
        file << line << "\n";
    }
    return true;
}

bool write_file(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << content;
    return true;
}

bool touch_file(const std::string& path) {
    if (utime(path.c_str(), nullptr) == 0) {
        return true;
    }
    std::ofstream file(path, std::ios::app);
    return file.is_open();
}

std::vector<std::string> glob_files(const std::string& pattern) {
    std::vector<std::string> results;
    glob_t glob_result;
    memset(&glob_result, 0, sizeof(glob_result));
    int ret = glob(pattern.c_str(), GLOB_TILDE | GLOB_NOSORT, nullptr, &glob_result);
    if (ret == 0) {
        for (size_t i = 0; i < glob_result.gl_pathc; ++i) {
            results.emplace_back(glob_result.gl_pathv[i]);
        }
    }
    globfree(&glob_result);
    std::sort(results.begin(), results.end());
    return results;
}

int count_dir_files(const std::string& dir, bool recursive) {
    std::error_code ec;
    if (!std::filesystem::exists(dir, ec)) return 0;
    int count = 0;
    if (recursive) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir, ec)) {
            if (entry.is_regular_file()) count++;
        }
    } else {
        for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
            if (entry.is_regular_file()) count++;
        }
    }
    return count;
}

bool check_output_path(const std::string& path) {
    if (path.empty()) return false;
    std::error_code ec;
    auto parent = std::filesystem::path(path).parent_path();
    if (!parent.empty() && !std::filesystem::exists(parent, ec)) {
        std::filesystem::create_directories(parent, ec);
    }
    return true;
}

std::unordered_map<std::string, std::string> load_config(const std::string& path) {
    std::unordered_map<std::string, std::string> config;
    auto lines = read_file(path);
    for (const auto& raw_line : lines) {
        auto line = trim(raw_line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        auto eq_pos = line.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = trim(line.substr(0, eq_pos));
            std::string val = trim(line.substr(eq_pos + 1));
            // strip surrounding quotes
            if (val.size() >= 2 && ((val.front() == '"' && val.back() == '"') ||
                                    (val.front() == '\'' && val.back() == '\''))) {
                val = val.substr(1, val.size() - 2);
            }
            config[key] = val;
        }
    }
    return config;
}

}  // namespace inxi::utils
