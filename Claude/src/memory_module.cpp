#include "memory_module.h"
#include "common.h"

#include <map>
#include <cctype>

namespace mod {
using namespace util;

namespace {
std::map<std::string, double> parseMemInfoKb() {
    std::map<std::string, double> out;
    for (const auto& line : readLines("/proc/meminfo")) {
        auto pos = line.find(':');
        if (pos == std::string::npos) continue;
        std::string key = trim(line.substr(0, pos));
        std::string rest = trim(line.substr(pos + 1));
        // rest looks like "4096540 kB"
        std::string numPart;
        for (char c : rest) {
            if (std::isdigit(static_cast<unsigned char>(c))) numPart += c;
            else if (!numPart.empty()) break;
        }
        if (!numPart.empty()) {
            try { out[key] = std::stod(numPart); } catch (...) {}
        }
    }
    return out;
}
} // namespace

MemInfo getMemInfo() {
    MemInfo m;
    auto kv = parseMemInfoKb();
    double totalKb = kv.count("MemTotal") ? kv["MemTotal"] : 0;
    double availKb;
    if (kv.count("MemAvailable")) {
        availKb = kv["MemAvailable"];
    } else {
        // Pre-3.14 kernel fallback approximation.
        availKb = (kv.count("MemFree") ? kv["MemFree"] : 0)
                + (kv.count("Buffers") ? kv["Buffers"] : 0)
                + (kv.count("Cached") ? kv["Cached"] : 0);
    }
    m.totalBytes = totalKb * 1024.0;
    m.availableBytes = availKb * 1024.0;
    m.usedBytes = m.totalBytes - m.availableBytes;
    if (m.usedBytes < 0) m.usedBytes = 0;
    return m;
}

void printMemorySection(std::ostream& os, bool color) {
    SectionPrinter p(os, color);
    p.header("Memory");

    MemInfo m = getMemInfo();
    if (m.totalBytes <= 0) {
        p.beginLine();
        p.field("Message", "No /proc/meminfo data available.");
        p.endLine();
        return;
    }

    p.beginLine();
    p.field("total", humanSize(m.totalBytes));
    p.field("used", humanSize(m.usedBytes) + " (" + percentStr(m.usedBytes, m.totalBytes) + ")");
    p.field("available", humanSize(m.availableBytes));
    p.endLine();
}

} // namespace mod
