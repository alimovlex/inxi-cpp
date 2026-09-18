#include "common.h"

#include <fstream>
#include <sstream>
#include <array>
#include <memory>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

namespace util {

std::string readFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

std::string readFirstLine(const std::string& path) {
    std::ifstream f(path);
    if (!f) return "";
    std::string line;
    std::getline(f, line);
    return trim(line);
}

std::vector<std::string> readLines(const std::string& path) {
    std::vector<std::string> out;
    std::ifstream f(path);
    if (!f) return out;
    std::string line;
    while (std::getline(f, line)) out.push_back(line);
    return out;
}

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        if (!item.empty()) out.push_back(item);
    }
    return out;
}

bool startsWith(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

std::string runCommand(const std::string& cmd) {
    std::array<char, 512> buffer{};
    std::string result;
    std::string fullCmd = cmd + " 2>/dev/null";
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(fullCmd.c_str(), "r"), pclose);
    if (!pipe) return "";
    size_t n;
    while ((n = fread(buffer.data(), 1, buffer.size(), pipe.get())) > 0) {
        result.append(buffer.data(), n);
    }
    return result;
}

bool commandExists(const std::string& name) {
    std::string check = "command -v " + name + " >/dev/null 2>&1";
    return std::system(check.c_str()) == 0;
}

std::string humanSize(double bytes, int decimals) {
    static const char* units[] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB"};
    int unit = 0;
    double val = bytes;
    while (val >= 1024.0 && unit < 5) {
        val /= 1024.0;
        unit++;
    }
    std::ostringstream ss;
    ss.setf(std::ios::fixed);
    ss.precision(unit == 0 ? 0 : decimals);
    ss << val << " " << units[unit];
    return ss.str();
}

std::string formatUptime(double seconds) {
    long total = static_cast<long>(seconds);
    long days = total / 86400;
    long hours = (total % 86400) / 3600;
    long mins = (total % 3600) / 60;
    std::ostringstream ss;
    if (days > 0) ss << days << "d ";
    if (days > 0 || hours > 0) ss << hours << "h ";
    ss << mins << "m";
    return ss.str();
}

std::string percentStr(double part, double whole, int decimals) {
    if (whole <= 0) return "N/A";
    double pct = part / whole * 100.0;
    std::ostringstream ss;
    ss.setf(std::ios::fixed);
    ss.precision(decimals);
    ss << pct << "%";
    return ss.str();
}

bool stdoutIsTty() {
    return isatty(fileno(stdout)) != 0;
}

std::string paint(const std::string& text, Color c, bool enabled) {
    if (!enabled) return text;
    const char* code = "\033[0m";
    switch (c) {
        case Color::Header: code = "\033[1;36m"; break; // bold cyan
        case Color::Label:  code = "\033[0;32m"; break; // green
        case Color::Value:  code = "\033[0m";    break; // terminal default
        case Color::Warn:   code = "\033[0;33m"; break; // yellow
        case Color::Reset:  code = "\033[0m";    break;
    }
    return std::string(code) + text + "\033[0m";
}

void SectionPrinter::header(const std::string& name) {
    os_ << paint(name + ":", Color::Header, color_) << "\n";
}

void SectionPrinter::beginLine(int indent) {
    os_ << std::string(indent, ' ');
    lineOpen_ = true;
    firstFieldOnLine_ = true;
}

void SectionPrinter::field(const std::string& label, const std::string& value) {
    if (!lineOpen_) beginLine();
    if (!firstFieldOnLine_) os_ << " ";
    os_ << paint(label + ":", Color::Label, color_) << " " << value;
    firstFieldOnLine_ = false;
}

void SectionPrinter::raw(const std::string& text) {
    if (!lineOpen_) beginLine();
    if (!firstFieldOnLine_) os_ << " ";
    os_ << text;
    firstFieldOnLine_ = false;
}

void SectionPrinter::endLine() {
    os_ << "\n";
    lineOpen_ = false;
}

} // namespace util
