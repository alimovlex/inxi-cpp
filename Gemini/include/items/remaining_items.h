// remaining_items.h
// Stub headers for the remaining InfoItem subclasses.
// Each covers a named section from the Perl source.
// Separate .cpp stubs are provided in src/items/ for each.
#pragma once

// -----------------------------------------------------------------------
// ProcessItem  (-I flag)
// Perl: process/ps_aux data, running process info
// -----------------------------------------------------------------------
#include "items/info_item.h"
#include <string>
#include <vector>

namespace inxi {

class ProcessItem final : public InfoItem {
public:
    explicit ProcessItem(GlobalState& s) : InfoItem(s) {}
    std::string label() const override { return "Processes"; }
    // Perl: get(), ps_data(), proc_data()
    std::vector<OutputRow> get() override;
};

// -----------------------------------------------------------------------
// RepoItem  (-r flag)
// Perl: repo_data(), package manager detection
// -----------------------------------------------------------------------
class RepoItem final : public InfoItem {
public:
    explicit RepoItem(GlobalState& s) : InfoItem(s) {}
    std::string label() const override { return "Repos"; }
    // Perl: get(), get_repos(), distro_repos()
    std::vector<OutputRow> get() override;
};

// -----------------------------------------------------------------------
// SwapItem  (-j flag)
// Perl: swap data from /proc/swaps and sysctl
// -----------------------------------------------------------------------
class SwapItem final : public InfoItem {
public:
    explicit SwapItem(GlobalState& s) : InfoItem(s) {}
    std::string label() const override { return "Swap"; }
    // Perl: get(), swap_data(), swap_output()
    std::vector<OutputRow> get() override;
};

// -----------------------------------------------------------------------
// UnmountedItem  (-o flag)
// Perl: unmounted / unformatted partition detection
// -----------------------------------------------------------------------
class UnmountedItem final : public InfoItem {
public:
    explicit UnmountedItem(GlobalState& s) : InfoItem(s) {}
    std::string label() const override { return "Unmounted"; }
    // Perl: get(), unmounted_data()
    std::vector<OutputRow> get() override;
};

// -----------------------------------------------------------------------
// UsbItem  (-J flag)
// Perl: USB tree, lsusb / usbconfig / usbdevs parsing
// -----------------------------------------------------------------------
class UsbItem final : public InfoItem {
public:
    explicit UsbItem(GlobalState& s) : InfoItem(s) {}
    std::string label() const override { return "USB"; }
    // Perl: get(), usb_data(), lsusb_data(), usbconfig_data()
    std::vector<OutputRow> get() override;
};

// -----------------------------------------------------------------------
// WeatherItem  (-w/-W flag)
// Perl: weather data download and format
// -----------------------------------------------------------------------
class WeatherItem final : public InfoItem {
public:
    explicit WeatherItem(GlobalState& s) : InfoItem(s) {}
    std::string label() const override { return "Weather"; }
    // Perl: get(), weather_data(), download_weather()
    std::vector<OutputRow> get() override;
};

// -----------------------------------------------------------------------
// DisplayItem  (internal, used by GraphicsItem for monitor/Xorg data)
// -----------------------------------------------------------------------
class DisplayItem final : public InfoItem {
public:
    explicit DisplayItem(GlobalState& s) : InfoItem(s) {}
    std::string label() const override { return "Display"; }
    // Perl: display_data() related subs in SystemDebugger::display_data()
    std::vector<OutputRow> get() override;
};

}  // namespace inxi
