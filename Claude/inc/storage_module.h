#pragma once
#include <ostream>

namespace mod {

void printDrivesSection(std::ostream& os, bool color);      // inxi -D
void printPartitionsSection(std::ostream& os, bool color);   // inxi -P
void printSwapSection(std::ostream& os, bool color);         // inxi -j

// Cheap getters reused by the default one-line summary.
double getTotalDriveBytes();
double getTotalUsedPartitionBytes();

} // namespace mod
