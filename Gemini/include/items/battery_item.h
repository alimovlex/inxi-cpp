// battery_item.h
// Collects battery / power-supply information.
// Perl source: package BatteryItem (lines ~8258–8803)
//
// Perl subs mapped:
//   get()                  → BatteryItem::get()
//   battery_output()       → BatteryItem::battery_output()
//   battery_data_sys()     → BatteryItem::battery_data_sys()
//   battery_data_sysctl()  → BatteryItem::battery_data_sysctl()
//   battery_data_dmi()     → BatteryItem::battery_data_dmi()
//   upower_data()          → BatteryItem::upower_data()
#pragma once
#include "items/info_item.h"
#include <string>
#include <vector>

namespace inxi {

struct BatteryData {
    int         id       = 0;
    std::string name;         // "BAT0"
    std::string status;       // "Charging"|"Discharging"|"Full"|"Unknown"
    int         charge   = 0; // percent
    float       wh       = 0; // Wh capacity
    std::string vendor;
    std::string model;
    std::string chemistry;
    std::string cycles;
    std::string serial;
    std::string voltage;
    std::string voltage_min;
    std::string condition; // "Good"|"Replace"|"12/41.4 Wh (28.9%)"
    bool        removable = false;
};

class BatteryItem final : public InfoItem {
public:
    explicit BatteryItem(GlobalState& s) : InfoItem(s) {}
    std::string label() const override { return "Battery"; }
    std::vector<OutputRow> get() override;

private:
    // Perl: battery_output() — formats battery rows
    std::vector<OutputRow> battery_output(const std::vector<BatteryData>& bats);

    // Perl: battery_data_sys() — reads /sys/class/power_supply/*
    std::vector<BatteryData> battery_data_sys();

    // Perl: battery_data_sysctl() — reads BSD sysctl acpi battery data
    std::vector<BatteryData> battery_data_sysctl();

    // Perl: battery_data_dmi() — falls back to DMI battery record
    std::vector<BatteryData> battery_data_dmi();

    // Perl: upower_data() — supplements with upower output
    void upower_data(std::vector<BatteryData>& bats);
};

}  // namespace inxi
