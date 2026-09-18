// string_utils.cpp
#include "utils/string_utils.h"
#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <iomanip>
#include <regex>
#include <sstream>

namespace inxi::utils {

std::string trim_left(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : s.substr(start);
}

std::string trim_right(const std::string& s) {
    size_t end = s.find_last_not_of(" \t\r\n");
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

std::string trim(const std::string& s) {
    return trim_left(trim_right(s));
}

std::string join(const std::vector<std::string>& v, const std::string& sep) {
    std::string res;
    for (size_t i = 0; i < v.size(); ++i) {
        if (i > 0) res += sep;
        res += v[i];
    }
    return res;
}

std::vector<std::string> split_lines(const std::string& s) {
    std::vector<std::string> lines;
    std::stringstream ss(s);
    std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
    }
    return lines;
}

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> tokens;
    std::stringstream ss(s);
    std::string token;
    while (std::getline(ss, token, delim)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::vector<std::string> split(const std::string& s, const std::string& delim) {
    std::vector<std::string> tokens;
    if (delim.empty()) {
        tokens.push_back(s);
        return tokens;
    }
    size_t start = 0;
    size_t end = s.find(delim);
    while (end != std::string::npos) {
        tokens.push_back(s.substr(start, end - start));
        start = end + delim.length();
        end = s.find(delim, start);
    }
    tokens.push_back(s.substr(start));
    return tokens;
}

std::vector<std::string> unique(const std::vector<std::string>& v) {
    return remove_duplicates(v);
}

std::vector<std::string> remove_duplicates(const std::vector<std::string>& v) {
    std::vector<std::string> result;
    for (const auto& item : v) {
        if (std::find(result.begin(), result.end(), item) == result.end()) {
            result.push_back(item);
        }
    }
    return result;
}

std::string get_piece(const std::string& s, int n, const std::string& sep) {
    auto pieces = split(s, sep);
    if (n >= 0 && n < static_cast<int>(pieces.size())) {
        return pieces[n];
    }
    return "";
}

std::string clean(std::string s) {
    if (s.empty()) return s;
    static const std::regex r(
        R"(\b(chipset|company|components|computing|computer|corporation|communications|electronics?|electrical|electric|group|incorporation|industrial|international|limited|revision|semiconductor|software|technologies|technology|ltd|inc|intl|co|corp)\b|\(tm\)|\(r\)|®|\(rev [^)]+\)|[,'"?*]|<[^>]+>)",
        std::regex::icase
    );
    s = std::regex_replace(s, r, " ");
    s = std::regex_replace(s, std::regex(R"(\s+)"), " ");
    return trim(s);
}

std::string strip_ansi(const std::string& s) {
    static const std::regex ansi_re(R"(\x1b\[[0-9;]*[a-zA-Z])");
    return std::regex_replace(s, ansi_re, "");
}

size_t visible_length(const std::string& s) {
    return strip_ansi(s).length();
}

std::string clean_characters(std::string s) {
    s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) {
        return c < 32 && c != '\t' && c != '\n' && c != '\r';
    }), s.end());
    return trim(s);
}

std::string clean_arm(std::string s) {
    s = std::regex_replace(s, std::regex(R"(Processor\s+rev\s+\d+.*)"), "");
    return trim(s);
}

std::string clean_disk(std::string s) {
    s = std::regex_replace(s, std::regex(R"(\s+Direct-Access\s*)"), "");
    s = std::regex_replace(s, std::regex(R"(\s+Disk\s*)"), "");
    return trim(s);
}

std::string clean_dmi(std::string s) {
    s = trim(s);
    std::string lower = to_lower(s);
    if (lower == "to be filled by o.e.m." || lower == "default string" ||
        lower == "none" || lower == "n/a" || lower == "not applicable" ||
        lower == "system manufacturer" || lower == "system product name" ||
        lower == "chassis manufacturer" || lower == "base board manufacturer" ||
        lower == "o.e.m." || lower == "type2 - board manufacturer") {
        return "";
    }
    return clean(s);
}

std::string clean_pci(std::string s) {
    s = std::regex_replace(s, std::regex(R"(^\d{2}:\d{2}\.\d\s+)"), "");
    s = std::regex_replace(s, std::regex(R"(\s*\(rev\s+[0-9a-fA-F]+\)$)"), "");
    return clean(s);
}

std::string clean_pci_subsystem(std::string s) {
    s = std::regex_replace(s, std::regex(R"(\s*\[.*\]$)"), "");
    return trim(s);
}

std::string clean_unset(const std::string& s) {
    std::string lower = to_lower(trim(s));
    if (lower == "unknown" || lower == "n/a" || lower == "none" || lower == "null" || lower == "empty") {
        return "";
    }
    return s;
}

std::string clean_regex(std::string s) {
    static const std::regex special_chars(R"([-[\]{}()*+?.,\^$|#\s])");
    return std::regex_replace(s, special_chars, R"(\$&)");
}

bool is_int(const std::string& s) {
    if (s.empty()) return false;
    size_t start = (s[0] == '-' || s[0] == '+') ? 1 : 0;
    if (start == s.size()) return false;
    for (size_t i = start; i < s.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    }
    return true;
}

bool is_numeric(const std::string& s) {
    if (s.empty()) return false;
    std::istringstream iss(s);
    double d;
    char c;
    return (iss >> d) && !(iss >> c);
}

bool is_hex(const std::string& s) {
    if (s.empty()) return false;
    std::string str = s;
    if (str.rfind("0x", 0) == 0 || str.rfind("0X", 0) == 0) {
        str = str.substr(2);
    }
    if (str.empty()) return false;
    for (char c : str) {
        if (!std::isxdigit(static_cast<unsigned char>(c))) return false;
    }
    return true;
}

uint64_t convert_hex(const std::string& hex_str) {
    if (hex_str.empty()) return 0;
    try {
        return std::stoull(hex_str, nullptr, 16);
    } catch (...) {
        return 0;
    }
}

std::string translate_size(uint64_t bytes, int decimals) {
    const char* units[] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB"};
    double b = static_cast<double>(bytes);
    int idx = 0;
    while (b >= 1024.0 && idx < 5) {
        b /= 1024.0;
        idx++;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimals) << b << " " << units[idx];
    return oss.str();
}

uint64_t parse_size(const std::string& human) {
    std::string str = trim(human);
    if (str.empty()) return 0;
    double val = 0.0;
    std::string unit;
    std::stringstream ss(str);
    ss >> val >> unit;
    unit = to_upper(trim(unit));
    uint64_t mult = 1;
    if (unit.find("K") != std::string::npos) mult = 1024ULL;
    else if (unit.find("M") != std::string::npos) mult = 1024ULL * 1024ULL;
    else if (unit.find("G") != std::string::npos) mult = 1024ULL * 1024ULL * 1024ULL;
    else if (unit.find("T") != std::string::npos) mult = 1024ULL * 1024ULL * 1024ULL * 1024ULL;
    else if (unit.find("P") != std::string::npos) mult = 1024ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL;
    return static_cast<uint64_t>(val * mult);
}

int compare_versions(const std::string& a, const std::string& b) {
    auto parts_a = split(a, '.');
    auto parts_b = split(b, '.');
    size_t n = std::max(parts_a.size(), parts_b.size());
    for (size_t i = 0; i < n; ++i) {
        int va = (i < parts_a.size() && is_int(parts_a[i])) ? std::stoi(parts_a[i]) : 0;
        int vb = (i < parts_b.size() && is_int(parts_b[i])) ? std::stoi(parts_b[i]) : 0;
        if (va < vb) return -1;
        if (va > vb) return 1;
    }
    return 0;
}

std::vector<std::string> regex_range(const std::string& range_expr) {
    std::vector<std::string> res;
    auto ranges = split(range_expr, ',');
    for (const auto& r : ranges) {
        auto dash = r.find('-');
        if (dash != std::string::npos) {
            int start = std::stoi(r.substr(0, dash));
            int end = std::stoi(r.substr(dash + 1));
            for (int i = start; i <= end; ++i) {
                res.push_back(std::to_string(i));
            }
        } else if (!r.empty()) {
            res.push_back(r);
        }
    }
    return res;
}

std::string get_size(uint64_t bytes) {
    return translate_size(bytes, 2);
}

std::string filter(const std::string& s) {
    return s.empty() ? "<filter>" : s;
}

std::string filter_partition(const std::string& s) {
    return s;
}

std::string filter_pci_long(const std::string& s) {
    return s.size() > 64 ? s.substr(0, 61) + "..." : s;
}

std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

std::string to_upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); });
    return s;
}

std::string format(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    va_list args_copy;
    va_copy(args_copy, args);
    int len = vsnprintf(nullptr, 0, fmt, args_copy);
    va_end(args_copy);
    if (len < 0) {
        va_end(args);
        return "";
    }
    std::string res(len, '\0');
    vsnprintf(&res[0], len + 1, fmt, args);
    va_end(args);
    return res;
}

}  // namespace inxi::utils
