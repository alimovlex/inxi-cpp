#pragma once
#include <ostream>
#include <string>

namespace mod {

struct CpuSummary {
    std::string model = "N/A";
    std::string vendor;
    int logicalCount = 0;     // threads, i.e. lines in /proc/cpuinfo
    int physicalCores = 0;    // best-effort unique physical/core id count
    double currentMHz = 0.0;  // first reported cpu MHz
};

// Cheap getter, reused by the default one-line summary so we don't parse
// /proc/cpuinfo twice.
CpuSummary getCpuSummary();

// inxi -C / --cpu. showAllFlags mirrors inxi's -f/--flags: by default the
// (very long) CPU flags list is summarized, not printed in full.
void printCpuSection(std::ostream& os, bool color, bool showAllFlags);

} // namespace mod
