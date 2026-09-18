// bluetooth_item.cpp
#include "items/bluetooth_item.h"
#include "utils/file_utils.h"
#include "utils/process_utils.h"
#include "utils/string_utils.h"
#include <filesystem>

namespace inxi {

std::vector<BluetoothAdapter> BluetoothItem::bt_adapter_data() {
    std::vector<BluetoothAdapter> adapters;
    auto dirs = utils::glob_files("/sys/class/bluetooth/hci*");
    int id = 1;
    for (const auto& d : dirs) {
        BluetoothAdapter a;
        a.id = "Device-" + std::to_string(id++);
        a.hci_id = std::filesystem::path(d).filename().string();
        a.mac = utils::trim(utils::read_file_str(d + "/address"));

        // Check canonical path of d + "/device"
        std::string dev_path;
        try {
            if (std::filesystem::exists(d + "/device")) {
                dev_path = std::filesystem::canonical(d + "/device").string();
            }
        } catch (...) {}

        if (!dev_path.empty() && dev_path.find("/usb") != std::string::npos) {
            a.is_usb = true;
            a.type = "USB";
            std::filesystem::path p(dev_path);
            std::filesystem::path usb_parent = p.parent_path();
            std::string vid = utils::trim(utils::read_file_str(usb_parent.string() + "/idVendor"));
            std::string pid = utils::trim(utils::read_file_str(usb_parent.string() + "/idProduct"));
            if (!vid.empty() && !pid.empty()) {
                a.chip_id = vid + ":" + pid;
                auto lsusb_out = utils::run_capture("lsusb -d " + a.chip_id);
                if (!lsusb_out.empty()) {
                    auto pos = lsusb_out[0].find(a.chip_id);
                    if (pos != std::string::npos) {
                        std::string desc = lsusb_out[0].substr(pos + a.chip_id.length());
                        a.name = utils::clean(desc);
                    }
                }
            }
            if (a.name.empty()) {
                std::string prod = utils::read_file_str(usb_parent.string() + "/product");
                if (!prod.empty()) a.name = utils::clean(prod);
            }

            std::string driver_link = dev_path + "/driver";
            try {
                if (std::filesystem::exists(driver_link)) {
                    a.driver = std::filesystem::canonical(driver_link).filename().string();
                }
            } catch (...) {}
            if (a.driver.empty()) a.driver = "btusb";

            std::string bus_id = usb_parent.filename().string();
            std::string devnum = utils::trim(utils::read_file_str(usb_parent.string() + "/devnum"));
            if (!bus_id.empty() && !devnum.empty()) {
                a.bus_id = bus_id + ":" + devnum;
            } else {
                a.bus_id = bus_id;
            }
        }

        if (a.name.empty()) {
            a.name = utils::trim(utils::read_file_str(d + "/name"));
            if (a.name.empty()) a.name = a.hci_id;
        }

        adapters.push_back(a);
    }
    rfkill_data(adapters);

    // Query btmgmt
    auto btmgmt_lines = utils::run_capture("btmgmt info");
    std::string cur_hci;
    for (const auto& line : btmgmt_lines) {
        if (line.rfind("hci", 0) == 0) {
            cur_hci = utils::get_piece(line, 0, ":");
        }
        for (auto& a : adapters) {
            if (a.hci_id == cur_hci) {
                if (line.find("addr ") != std::string::npos) {
                    auto pos = line.find("addr ");
                    auto rest = line.substr(pos + 5);
                    a.mac = utils::get_piece(rest, 0);
                }
                if (line.find("current settings:") != std::string::npos) {
                    if (line.find("powered") != std::string::npos) {
                        a.state = "up";
                    } else {
                        a.state = "down";
                    }
                }
                a.report_tool = "btmgmt";
            }
        }
    }

    return adapters;
}

std::vector<BluetoothAdapter> BluetoothItem::btmgmt_data() { return {}; }
std::vector<BluetoothAdapter> BluetoothItem::hciconfig_data() { return {}; }

void BluetoothItem::rfkill_data(std::vector<BluetoothAdapter>& adapters) {
    auto lines = utils::run_capture("rfkill -n -o TYPE,SOFT,HARD");
    for (const auto& l : lines) {
        if (l.find("bluetooth") != std::string::npos) {
            if (l.find("blocked") != std::string::npos) {
                for (auto& a : adapters) a.rfkill_blocked = true;
            }
        }
    }
}

std::string BluetoothItem::check_service() { return ""; }
std::string BluetoothItem::bluetooth_version(const std::string& raw) { (void)raw; return ""; }

std::vector<BluetoothAdapter> BluetoothItem::set_bluetooth_data() {
    return bt_adapter_data();
}

std::vector<OutputRow> BluetoothItem::device_output(const std::vector<BluetoothAdapter>& adapters) {
    std::vector<OutputRow> rows;
    bool is_first = true;
    for (const auto& a : adapters) {
        std::vector<Field> fields;
        fields.push_back(field(a.id, a.name));
        if (!a.driver.empty()) fields.push_back(field("driver", a.driver));
        if (!a.type.empty()) fields.push_back(field("type", a.type));
        if (!a.bus_id.empty()) fields.push_back(field("bus-ID", a.bus_id));
        if (!a.chip_id.empty()) fields.push_back(field("chip-ID", a.chip_id));

        if (is_first) {
            rows.push_back(make_row("Bluetooth", fields));
            is_first = false;
        } else {
            rows.push_back(make_row("", fields, true));
        }

        if (!a.report_tool.empty()) {
            std::vector<Field> r_fields;
            r_fields.push_back(field("Report", a.report_tool));
            r_fields.push_back(field("ID", a.hci_id));
            r_fields.push_back(field("state", a.state.empty() ? (a.rfkill_blocked ? "blocked" : "up") : a.state));
            if (!a.mac.empty()) {
                r_fields.push_back(field("address", filter_data(a.mac)));
            }
            rows.push_back(make_row("", r_fields, true));
        }
    }
    return rows;
}

std::vector<OutputRow> BluetoothItem::usb_output(const std::vector<BluetoothAdapter>& adapters) { (void)adapters; return {}; }
std::vector<OutputRow> BluetoothItem::advanced_output(const std::vector<BluetoothAdapter>& adapters) { (void)adapters; return {}; }

std::vector<OutputRow> BluetoothItem::get() {
    auto adapters = set_bluetooth_data();
    return device_output(adapters);
}

}  // namespace inxi
