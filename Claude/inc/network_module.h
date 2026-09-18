#pragma once
#include <ostream>

namespace mod {

// inxi -N/-n : network interfaces, driver, state, speed, MAC.
// filterMac mirrors inxi's -z/--filter (inxi's default is unfiltered on a
// real terminal, and filtered when it detects it's running in an IRC
// client); here it's simply off unless the caller passes true.
void printNetworkSection(std::ostream& os, bool color, bool filterMac);

} // namespace mod
