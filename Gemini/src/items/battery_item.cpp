// battery_item.cpp
#include "items/battery_item.h"
#include "utils/file_utils.h"
#include "utils/string_utils.h"
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace inxi {

std::vector<BatteryData> BatteryItem::battery_data_sys() {
    std::vector<BatteryData> bats;
    auto dirs = utils::glob_files("/sys/class/power_supply/BAT*");
    int id = 1;
    for (const auto& dir : dirs) {
        BatteryData b;
        b.id = id++;
        b.name = std::filesystem::path(dir).filename().string();
        b.status = utils::to_lower(utils::trim(utils::read_file_str(dir + "/status")));

        std::string mfr = utils::clean(utils::read_file_str(dir + "/manufacturer"));
        std::string model = utils::clean(utils::read_file_str(dir + "/model_name"));
        b.model = utils::trim(mfr + " " + model);
        if (b.model.empty()) b.model = b.name;

        b.chemistry = utils::trim(utils::read_file_str(dir + "/technology"));
        b.serial = utils::trim(utils::read_file_str(dir + "/serial_number"));

        // Voltage
        std::string volt_str = utils::read_file_str(dir + "/voltage_now");
        std::string volt_min_str = utils::read_file_str(dir + "/voltage_min_design");
        double volt_v = 0;
        if (utils::is_numeric(volt_str)) {
            volt_v = std::stod(volt_str) / 1e6;
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(2) << volt_v;
            b.voltage = ss.str();
        }
        double volt_min_v = 0;
        if (utils::is_numeric(volt_min_str)) {
            volt_min_v = std::stod(volt_min_str) / 1e6;
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(1) << volt_min_v;
            b.voltage_min = ss.str();
        }

        // Capacity / charge percentage
        std::string cap_str = utils::read_file_str(dir + "/capacity");
        if (utils::is_int(cap_str)) {
            b.charge = std::stoi(cap_str);
        }

        // Energy / capacity Wh
        std::string energy_now_str = utils::read_file_str(dir + "/energy_now");
        std::string energy_full_str = utils::read_file_str(dir + "/energy_full");
        std::string energy_design_str = utils::read_file_str(dir + "/energy_full_design");

        std::string chg_now_str = utils::read_file_str(dir + "/charge_now");
        std::string chg_full_str = utils::read_file_str(dir + "/charge_full");
        std::string chg_design_str = utils::read_file_str(dir + "/charge_full_design");

        double volt_mult = (volt_min_v > 0) ? volt_min_v : ((volt_v > 0) ? volt_v : 1.0);

        if (utils::is_numeric(energy_now_str)) {
            b.wh = static_cast<float>(std::stod(energy_now_str) / 1e6);
        } else if (utils::is_numeric(chg_now_str)) {
            b.wh = static_cast<float>((std::stod(chg_now_str) / 1e6) * volt_mult);
        }

        double full_wh = 0, design_wh = 0;
        if (utils::is_numeric(energy_full_str) && utils::is_numeric(energy_design_str)) {
            full_wh = std::stod(energy_full_str) / 1e6;
            design_wh = std::stod(energy_design_str) / 1e6;
        } else if (utils::is_numeric(chg_full_str) && utils::is_numeric(chg_design_str)) {
            full_wh = (std::stod(chg_full_str) / 1e6) * volt_mult;
            design_wh = (std::stod(chg_design_str) / 1e6) * volt_mult;
        }

        if (design_wh > 0) {
            double pct = (full_wh / design_wh) * 100.0;
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(1) << full_wh << "/" << design_wh
               << " Wh (" << std::fixed << std::setprecision(1) << pct << "%)";
            b.condition = ss.str();
        }

        bats.push_back(b);
    }
    return bats;
}

std::vector<BatteryData> BatteryItem::battery_data_sysctl() { return {}; }
std::vector<BatteryData> BatteryItem::battery_data_dmi() { return {}; }
void BatteryItem::upower_data(std::vector<BatteryData>& bats) { (void)bats; }

std::vector<OutputRow> BatteryItem::battery_output(const std::vector<BatteryData>& bats) {
    std::vector<OutputRow> rows;
    bool is_first = true;
    for (const auto& b : bats) {
        std::vector<Field> fields;
        fields.push_back(field("ID-" + std::to_string(b.id), b.name));

        std::ostringstream chg_ss;
        chg_ss << std::fixed << std::setprecision(1) << b.wh << " Wh ("
               << b.charge << "%)";
        fields.push_back(field("charge", chg_ss.str()));
        if (!b.condition.empty()) {
            fields.push_back(field("condition", b.condition));
        }
        if (state_.extra >= 1) {
            if (!b.voltage.empty()) fields.push_back(field("volts", b.voltage));
            if (!b.voltage_min.empty()) fields.push_back(field("min", b.voltage_min));
            if (!b.model.empty()) fields.push_back(field("model", b.model));
            if (!b.chemistry.empty()) fields.push_back(field("type", b.chemistry));
            if (!b.serial.empty()) fields.push_back(field("serial", filter_data(b.serial)));
        }
        if (!b.status.empty()) {
            fields.push_back(field("status", b.status));
        }

        if (is_first) {
            rows.push_back(make_row("Battery", fields));
            is_first = false;
        } else {
            rows.push_back(make_row("", fields, true));
        }
    }
    return rows;
}

std::vector<OutputRow> BatteryItem::get() {
    auto bats = battery_data_sys();
    return battery_output(bats);
}

}  // namespace inxi
