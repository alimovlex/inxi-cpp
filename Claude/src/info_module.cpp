#include "info_module.h"
#include "common.h"
#include "memory_module.h"
#include "version.h"

#include <filesystem>
#include <cstdlib>
#include <cctype>
#include <algorithm>

namespace fs = std::filesystem;

namespace mod {
using namespace util;

double getUptimeSeconds() {
    std::string first = readFirstLine("/proc/uptime");
    auto spacePos = first.find(' ');
    std::string secs = spacePos == std::string::npos ? first : first.substr(0, spacePos);
    try { return std::stod(secs); } catch (...) { return 0; }
}

long getProcessCount() {
    long count = 0;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator("/proc", ec)) {
        std::string name = entry.path().filename().string();
        if (!name.empty() && std::all_of(name.begin(), name.end(),
                [](unsigned char c) { return std::isdigit(c); })) {
            count++;
        }
    }
    return count;
}

std::string getPackageInfo() {
    if (commandExists("dpkg-query")) {
        std::string out = trim(runCommand("dpkg-query -f '.' -W"));
        if (!out.empty()) return std::to_string(out.size()) + " (dpkg)";
    }
    if (fs::exists("/var/lib/dpkg/status")) {
        long n = 0;
        for (auto& line : readLines("/var/lib/dpkg/status")) {
            if (startsWith(line, "Package: ")) n++;
        }
        if (n > 0) return std::to_string(n) + " (dpkg)";
    }
    if (commandExists("rpm")) {
        std::string out = trim(runCommand("rpm -qa"));
        if (!out.empty()) return std::to_string(split(out, '\n').size()) + " (rpm)";
    }
    if (commandExists("pacman")) {
        std::string out = trim(runCommand("pacman -Qq"));
        if (!out.empty()) return std::to_string(split(out, '\n').size()) + " (pacman)";
    }
    if (commandExists("apk")) {
        std::string out = trim(runCommand("apk info"));
        if (!out.empty()) return std::to_string(split(out, '\n').size()) + " (apk)";
    }
    return "N/A";
}

std::string getShellName() {
    const char* shell = std::getenv("SHELL");
    if (!shell || !*shell) return "N/A";
    return fs::path(shell).filename().string();
}

std::string getCompilerVersion() {
    if (commandExists("gcc")) {
        std::string v = trim(runCommand("gcc -dumpversion"));
        if (!v.empty()) return "gcc " + v;
    }
    if (commandExists("clang")) {
        std::string out = runCommand("clang --version");
        auto lines = split(out, '\n');
        if (!lines.empty()) return trim(lines[0]);
    }
    return "N/A";
}

void printInfoSection(std::ostream& os, bool color) {
    SectionPrinter p(os, color);
    p.header("Info");

    MemInfo mem = getMemInfo();

    p.beginLine();
    p.field("Processes", std::to_string(getProcessCount()));
    p.field("Uptime", formatUptime(getUptimeSeconds()));
    if (mem.totalBytes > 0) {
        p.field("Memory", humanSize(mem.usedBytes) + " / " + humanSize(mem.totalBytes) +
                           " (" + percentStr(mem.usedBytes, mem.totalBytes) + ")");
    }
    p.endLine();

    p.beginLine();
    p.field("Shell", getShellName());
    p.field("Packages", getPackageInfo());
    p.field("Compiler", getCompilerVersion());
    p.endLine();

    p.beginLine();
    p.field(PROGRAM_NAME, PROGRAM_VERSION);
    p.raw("(partial C++ port of inxi)");
    p.endLine();
}

} // namespace mod
