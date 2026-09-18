// sensor_item.h
// Collects hardware sensor temperatures / voltages / fan speeds.
// Perl: sensors-related subs
//
// Perl subs mapped:
//   get()            → SensorItem::get()
//   full_output()    → SensorItem::full_output()
//   short_output()   → SensorItem::short_output()
//   sensor_data()    → SensorItem::sensor_data()
//   sensors_data()   → SensorItem::sensors_data()    [lm-sensors]
//   sysfs_data()     → SensorItem::sysfs_data()      [/sys/class/hwmon]
//   bsd_sensor_data()→ SensorItem::bsd_sensor_data() [sysctl]
#pragma once
#include "items/info_item.h"
#include <string>
#include <vector>

namespace inxi {

enum class SensorType { Temperature, Voltage, Fan, Power, Current, Unknown };

struct SensorReading {
    std::string chip;      // adapter/chip name
    std::string feature;   // "Core 0"|"fan1"|"in0"
    SensorType  type       = SensorType::Unknown;
    float       value      = 0.0f;
    float       high       = 0.0f;  // threshold
    float       crit       = 0.0f;
    std::string unit;               // "°C"|"V"|"RPM"|"W"
    bool        alarm      = false;
};

struct SensorChip {
    std::string name;
    std::string adapter;
    std::vector<SensorReading> readings;
};

class SensorItem final : public InfoItem {
public:
    explicit SensorItem(GlobalState& s) : InfoItem(s) {}
    std::string label() const override { return "Sensors"; }
    std::vector<OutputRow> get() override;

private:
    std::vector<OutputRow>  full_output(const std::vector<SensorChip>& chips);
    std::vector<OutputRow>  short_output(const std::vector<SensorChip>& chips);

    std::vector<SensorChip> sensor_data();
    std::vector<SensorChip> sensors_data();       // lm-sensors
    std::vector<SensorChip> sysfs_data();         // /sys/class/hwmon
    std::vector<SensorChip> bsd_sensor_data();    // sysctl dev.*
};

}  // namespace inxi
