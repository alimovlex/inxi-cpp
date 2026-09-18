#include "cpu_module.h"
#include "common.h"
#include "system_module.h"  // reuse getKernelArch() for the bits: field

#include <map>
#include <set>
#include <utility>
#include <vector>
#include <cctype>
#include <initializer_list>

namespace mod {
using namespace util;

namespace {

using Block = std::map<std::string, std::string>;

// /proc/cpuinfo is one block of "key\t: value" lines per logical CPU,
// blocks separated by a blank line.
std::vector<Block> parseCpuInfoBlocks() {
    std::vector<Block> blocks;
    Block current;
    for (const auto& line : readLines("/proc/cpuinfo")) {
        if (trim(line).empty()) {
            if (!current.empty()) {
                blocks.push_back(current);
                current.clear();
            }
            continue;
        }
        auto pos = line.find(':');
        if (pos == std::string::npos) continue;
        std::string key = trim(line.substr(0, pos));
        std::string val = trim(line.substr(pos + 1));
        current[key] = val;
    }
    if (!current.empty()) blocks.push_back(current);
    return blocks;
}

std::string getField(const Block& b, std::initializer_list<const char*> keys) {
    for (const char* k : keys) {
        auto it = b.find(k);
        if (it != b.end()) return it->second;
    }
    return "";
}

// "48K" / "266240K" / "1M" -> bytes.
double parseSizeSuffix(const std::string& s) {
    if (s.empty()) return 0;
    std::string numPart;
    char suffix = 0;
    for (char c : s) {
        if (std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
            numPart += c;
        } else if (std::isalpha(static_cast<unsigned char>(c))) {
            suffix = static_cast<char>(std::toupper(c));
            break;
        }
    }
    if (numPart.empty()) return 0;
    double val = std::stod(numPart);
    switch (suffix) {
        case 'K': return val * 1024.0;
        case 'M': return val * 1024.0 * 1024.0;
        case 'G': return val * 1024.0 * 1024.0 * 1024.0;
        default:  return val;
    }
}

struct CacheInfo {
    double l1 = 0, l2 = 0, l3 = 0;
    bool found = false;
};

// Real hardware exposes per-level cache info under sysfs; this is more
// reliable than /proc/cpuinfo's single "cache size" field (x86-only, and
// only ever reports one level).
CacheInfo readCacheSysfs() {
    CacheInfo c;
    for (int i = 0; i < 8; ++i) {
        std::string base = "/sys/devices/system/cpu/cpu0/cache/index" + std::to_string(i);
        std::string level = readFirstLine(base + "/level");
        if (level.empty()) break;
        std::string size = readFirstLine(base + "/size");
        double bytes = parseSizeSuffix(size);
        c.found = true;
        if (level == "1") c.l1 += bytes;       // Data + Instruction combined
        else if (level == "2") c.l2 += bytes;
        else if (level == "3") c.l3 += bytes;
    }
    return c;
}

std::string freqPath(const std::string& file) {
    return "/sys/devices/system/cpu/cpu0/cpufreq/" + file;
}

} // namespace

CpuSummary getCpuSummary() {
    CpuSummary s;
    auto blocks = parseCpuInfoBlocks();
    if (blocks.empty()) return s;

    s.logicalCount = static_cast<int>(blocks.size());
    s.model = getField(blocks[0], {"model name", "Hardware", "model", "cpu model"});
    if (s.model.empty()) s.model = "N/A";
    s.vendor = getField(blocks[0], {"vendor_id", "CPU implementer"});

    std::string mhz = getField(blocks[0], {"cpu MHz"});
    if (!mhz.empty()) {
        try { s.currentMHz = std::stod(mhz); } catch (...) {}
    } else {
        std::string cur = readFirstLine(freqPath("scaling_cur_freq"));
        if (!cur.empty()) {
            try { s.currentMHz = std::stod(cur) / 1000.0; } catch (...) {}
        }
    }

    // Physical core count: count unique (physical id, core id) pairs when
    // the kernel exposes topology; otherwise fall back to "cpu cores", and
    // finally to the logical (thread) count.
    std::set<std::pair<std::string, std::string>> uniqueCores;
    bool haveTopology = true;
    for (auto& b : blocks) {
        std::string phys = getField(b, {"physical id"});
        std::string core = getField(b, {"core id"});
        if (phys.empty() || core.empty()) { haveTopology = false; break; }
        uniqueCores.insert({phys, core});
    }
    if (haveTopology && !uniqueCores.empty()) {
        s.physicalCores = static_cast<int>(uniqueCores.size());
    } else {
        std::string coresField = getField(blocks[0], {"cpu cores"});
        if (!coresField.empty()) {
            try { s.physicalCores = std::stoi(coresField); }
            catch (...) { s.physicalCores = s.logicalCount; }
        } else {
            s.physicalCores = s.logicalCount;
        }
    }
    return s;
}

void printCpuSection(std::ostream& os, bool color, bool showAllFlags) {
    SectionPrinter p(os, color);
    p.header("CPU");

    auto blocks = parseCpuInfoBlocks();
    if (blocks.empty()) {
        p.beginLine();
        p.field("Message", "No /proc/cpuinfo data available.");
        p.endLine();
        return;
    }

    CpuSummary s = getCpuSummary();
    std::string bits = getKernelArch().find("64") != std::string::npos ? "64" : "32";

    p.beginLine();
    p.field("Model", s.model);
    p.field("vendor", s.vendor.empty() ? "N/A" : s.vendor);
    p.field("bits", bits);
    p.field("cores", std::to_string(s.physicalCores));
    p.field("threads", std::to_string(s.logicalCount));
    p.endLine();

    p.beginLine();
    p.field("Speed (MHz)", s.currentMHz > 0 ? std::to_string(static_cast<long>(s.currentMHz)) : "N/A");
    std::string minF = readFirstLine(freqPath("scaling_min_freq"));
    std::string maxF = readFirstLine(freqPath("scaling_max_freq"));
    if (!minF.empty() && !maxF.empty()) {
        try {
            long mn = std::stol(minF) / 1000;
            long mx = std::stol(maxF) / 1000;
            p.field("min/max", std::to_string(mn) + "/" + std::to_string(mx));
        } catch (...) {
            p.field("min/max", "N/A");
        }
    } else {
        p.field("min/max", "N/A");
    }
    p.endLine();

    CacheInfo cache = readCacheSysfs();
    if (cache.found && (cache.l1 > 0 || cache.l2 > 0 || cache.l3 > 0)) {
        p.beginLine();
        p.raw("Cache:");
        if (cache.l1 > 0) p.field("L1", humanSize(cache.l1));
        if (cache.l2 > 0) p.field("L2", humanSize(cache.l2));
        if (cache.l3 > 0) p.field("L3", humanSize(cache.l3));
        p.endLine();
    } else {
        std::string legacyCache = getField(blocks[0], {"cache size"});
        if (!legacyCache.empty()) {
            p.beginLine();
            p.field("Cache", legacyCache);
            p.endLine();
        }
    }

    std::string flagsRaw = getField(blocks[0], {"flags", "Features"});
    auto flags = split(flagsRaw, ' ');
    p.beginLine();
    if (showAllFlags && !flags.empty()) {
        p.field("Flags", flagsRaw);
    } else {
        p.field("Flags", std::to_string(flags.size()) + " total (use --flags to show all)");
    }
    p.endLine();
}

} // namespace mod
