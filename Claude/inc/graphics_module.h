#pragma once
#include <ostream>

namespace mod {

// inxi -G / --graphics. Device listing needs lspci; resolution needs
// xrandr under X11. Wayland resolution detection (which real inxi gets
// from compositor-specific IPC) is out of scope for this port -- see
// README for what's not yet covered.
void printGraphicsSection(std::ostream& os, bool color);

} // namespace mod
