#pragma once
#include <ostream>
#include <string>

namespace mod {

// inxi -S / --system : host name, kernel, arch, desktop, distro.
void printSystemSection(std::ostream& os, bool color);

// inxi -M / --machine : DMI data - system/board/BIOS vendor+model.
// Requires /sys/class/dmi/id (usually root-readable only on some systems;
// world-readable on most modern desktop/laptop kernels). Degrades to a
// clear "no data" message on VMs/containers that don't expose it, same
// as inxi does.
void printMachineSection(std::ostream& os, bool color);

// Small getters reused by the one-line default summary.
std::string getKernelRelease();   // uname -r equivalent
std::string getKernelArch();      // uname -m equivalent
std::string getHostname();
std::string getDistroPretty();    // PRETTY_NAME from /etc/os-release

} // namespace mod
