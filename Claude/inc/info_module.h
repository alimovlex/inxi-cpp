#pragma once
#include <ostream>
#include <string>

namespace mod {

double getUptimeSeconds();
long getProcessCount();
std::string getPackageInfo();     // e.g. "1834 (dpkg)"
std::string getShellName();
std::string getCompilerVersion();

// inxi -I / --info
void printInfoSection(std::ostream& os, bool color);

} // namespace mod
