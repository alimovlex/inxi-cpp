// sensor_item.cpp
#include "items/sensor_item.h"
#include "utils/file_utils.h"
#include "utils/string_utils.h"
#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <set>
#include <sstream>

namespace inxi {

std::vector<SensorChip> SensorItem::sysfs_data() {
    std::vector<SensorChip> chips;
    auto hwmon_dirs = utils::glob_files("/sys/class/hwmon/hwmon*");
    for (const auto& dir : hwmon_dirs) {
        SensorChip chip;
        chip.name = utils::trim(utils::read_file_str(dir + "/name"));
        if (chip.name.empty()) continue;

        auto temp_files = utils::glob_files(dir + "/temp*_input");
        for (const auto& tf : temp_files) {
            std::string val_str = utils::trim(utils::read_file_str(tf));
            if (!utils::is_numeric(val_str)) continue;
            SensorReading r;
            r.chip = chip.name;
            r.type = SensorType::Temperature;
            r.value = static_cast<float>(std::stod(val_str) / 1000.0);
            r.unit = "C";

            // Label
            std::string base = tf.substr(0, tf.find("_input"));
            std::string lbl = utils::trim(utils::read_file_str(base + "_label"));
            r.feature = lbl.empty() ? std::filesystem::path(base).filename().string() : lbl;

            // Check for high/crit thresholds
            std::string high_str = utils::trim(utils::read_file_str(base + "_max"));
            if (!high_str.empty() && utils::is_numeric(high_str)) {
                r.high = static_cast<float>(std::stod(high_str) / 1000.0);
            }
            std::string crit_str = utils::trim(utils::read_file_str(base + "_crit"));
            if (!crit_str.empty() && utils::is_numeric(crit_str)) {
                r.crit = static_cast<float>(std::stod(crit_str) / 1000.0);
            }
            chip.readings.push_back(r);
        }

        auto fan_files = utils::glob_files(dir + "/fan*_input");
        for (const auto& ff : fan_files) {
            std::string val_str = utils::trim(utils::read_file_str(ff));
            if (!utils::is_numeric(val_str)) continue;
            SensorReading r;
            r.chip = chip.name;
            r.type = SensorType::Fan;
            r.value = static_cast<float>(std::stod(val_str));
            r.unit = "RPM";
            std::string base = ff.substr(0, ff.find("_input"));
            std::string lbl = utils::trim(utils::read_file_str(base + "_label"));
            r.feature = lbl.empty() ? std::filesystem::path(base).filename().string() : lbl;
            chip.readings.push_back(r);
        }

        auto volt_files = utils::glob_files(dir + "/in*_input");
        for (const auto& vf : volt_files) {
            std::string val_str = utils::trim(utils::read_file_str(vf));
            if (!utils::is_numeric(val_str)) continue;
            SensorReading r;
            r.chip = chip.name;
            r.type = SensorType::Voltage;
            r.value = static_cast<float>(std::stod(val_str) / 1000.0);
            r.unit = "V";
            std::string base = vf.substr(0, vf.find("_input"));
            std::string lbl = utils::trim(utils::read_file_str(base + "_label"));
            r.feature = lbl.empty() ? std::filesystem::path(base).filename().string() : lbl;
            chip.readings.push_back(r);
        }

        if (!chip.readings.empty()) {
            chips.push_back(chip);
        }
    }
    return chips;
}

std::vector<SensorChip> SensorItem::sensors_data() { return {}; }
std::vector<SensorChip> SensorItem::bsd_sensor_data() { return {}; }

std::vector<SensorChip> SensorItem::sensor_data() {
    return sysfs_data();
}

std::vector<OutputRow> SensorItem::short_output(const std::vector<SensorChip>& chips) {
    return full_output(chips);
}

std::vector<OutputRow> SensorItem::full_output(const std::vector<SensorChip>& chips) {
    std::vector<OutputRow> rows;

    std::vector<Field> temp_fields;
    std::vector<Field> fan_fields;
    std::vector<Field> volt_fields;

    std::set<std::string> seen_temp_labels;
    std::set<std::string> seen_fan_labels;

    for (const auto& c : chips) {
        for (const auto& r : c.readings) {
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(1) << r.value << " " << r.unit;
            if (r.type == SensorType::Temperature) {
                std::string label;
                if (c.name.find("pch") != std::string::npos) {
                    label = "pch";
                } else if (c.name.find("dell_smm") != std::string::npos) {
                    if (r.feature == "CPU") label = "cpu";
                    else if (r.feature == "Ambient") label = "mobo";
                    else if (state_.extra >= 2) label = utils::to_lower(r.feature);
                } else if (c.name.find("coretemp") != std::string::npos) {
                    if (r.feature.find("Package") != std::string::npos) {
                        label = "cpu";
                    } else if (state_.extra >= 2) {
                        // per-core: show as "Core 0", "Core 1" etc at -xx
                        label = r.feature;
                    }
                    // at extra < 2 skip individual cores entirely
                } else if (c.name.find("acpitz") != std::string::npos) {
                    label = "mobo";
                } else if (c.name.find("iwlwifi") != std::string::npos) {
                    if (state_.extra >= 1) label = "wifi";
                } else {
                    if (state_.extra >= 2) label = r.feature;
                }

                if (!label.empty() && seen_temp_labels.insert(label).second) {
                    temp_fields.push_back(field(label, ss.str()));
                }
            } else if (r.type == SensorType::Fan) {
                std::string label;
                if (r.feature.find("Processor") != std::string::npos ||
                    r.feature.find("fan1") != std::string::npos ||
                    r.feature.rfind("fan", 0) == 0) {
                    label = "cpu";
                } else {
                    label = utils::to_lower(r.feature);
                }
                // Only record non-zero fan speeds
                if (r.value > 0.0f && seen_fan_labels.insert(label).second) {
                    fan_fields.push_back(field(label, std::to_string(static_cast<int>(r.value))));
                }
            } else if (r.type == SensorType::Voltage && state_.extra >= 3) {
                volt_fields.push_back(field(r.feature, ss.str()));
            }
        }
    }

    if (!temp_fields.empty()) {
        // Sort: cpu first, then pch, mobo, others
        auto priority = [](const Field& f) -> int {
            if (f.key == "cpu")  return 0;
            if (f.key == "pch")  return 1;
            if (f.key == "mobo") return 2;
            return 3;
        };
        std::stable_sort(temp_fields.begin(), temp_fields.end(),
            [&priority](const Field& a, const Field& b) {
                return priority(a) < priority(b);
            });

        std::vector<Field> row_fields;
        row_fields.push_back(field("Src", "/sys"));
        row_fields.push_back(field("System Temperatures", ""));
        for (const auto& f : temp_fields) {
            row_fields.push_back(f);
        }
        rows.push_back(make_row("Sensors", row_fields));
    }

    // Fan speeds — always emit the row if any fan was detected (even if all 0)
    // but show N/A when no non-zero reading was found
    {
        // Count total fans detected (including zero-speed)
        std::set<std::string> all_fan_labels_seen;
        for (const auto& c : chips) {
            for (const auto& r : c.readings) {
                if (r.type == SensorType::Fan) all_fan_labels_seen.insert(r.feature);
            }
        }
        bool has_fans = !all_fan_labels_seen.empty();
        if (has_fans) {
            std::vector<Field> row_fields;
            row_fields.push_back(field("Fan Speeds (rpm)", ""));
            if (fan_fields.empty()) {
                row_fields.push_back(field("N/A", ""));
            } else {
                for (const auto& f : fan_fields) {
                    row_fields.push_back(f);
                }
            }
            rows.push_back(make_row("", row_fields, true));
        }
    }

    if (!volt_fields.empty()) {
        std::vector<Field> row_fields;
        row_fields.push_back(field("Voltages", ""));
        for (const auto& f : volt_fields) {
            row_fields.push_back(f);
        }
        rows.push_back(make_row("", row_fields, true));
    }

    return rows;
}

std::vector<OutputRow> SensorItem::get() {
    auto chips = sensor_data();
    return full_output(chips);
}

}  // namespace inxi
