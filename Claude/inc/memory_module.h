#pragma once
#include <ostream>

namespace mod {

struct MemInfo {
    double totalBytes = 0;
    double availableBytes = 0;
    double usedBytes = 0; // totalBytes - availableBytes
};

// Cheap getter, reused by the default summary and the Info section.
MemInfo getMemInfo();

// inxi -m / --memory (this port does not include DIMM slot/speed detail,
// which on real inxi requires root + dmidecode; see README).
void printMemorySection(std::ostream& os, bool color);

} // namespace mod
