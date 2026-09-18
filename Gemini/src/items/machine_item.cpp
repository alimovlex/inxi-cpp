// machine_item.cpp
#include "items/machine_item.h"
#include "utils/file_utils.h"
#include "utils/string_utils.h"
#include <filesystem>
#include <unistd.h>

namespace inxi {

std::string MachineItem::chassis_data(const std::string& raw_type) {
    int type_num = utils::is_int(raw_type) ? std::stoi(raw_type) : 0;
    switch (type_num) {
        case 3: return "Desktop";
        case 8: case 9: case 10: case 14: return "Laptop";
        case 11: return "Handheld";
        case 17: case 23: return "Server";
        case 30: case 31: case 32: return "Tablet";
        default: return "System";
    }
}

void MachineItem::sys_data(MachineData& m) {
    std::string dmi_path = "/sys/class/dmi/id/";
    if (!std::filesystem::exists(dmi_path)) {
        dmi_path = "/sys/devices/virtual/dmi/id/";
    }
    if (!std::filesystem::exists(dmi_path)) return;

    m.system_vendor = utils::clean_dmi(utils::read_file_str(dmi_path + "sys_vendor"));
    m.product_name = utils::clean_dmi(utils::read_file_str(dmi_path + "product_name"));
    m.product_version = utils::clean_dmi(utils::read_file_str(dmi_path + "product_version"));
    if (m.product_version.empty()) m.product_version = "N/A";

    if (geteuid() == 0) {
        m.product_serial = utils::clean_dmi(utils::read_file_str(dmi_path + "product_serial"));
        m.board_serial = utils::clean_dmi(utils::read_file_str(dmi_path + "board_serial"));
    } else {
        m.product_serial = "<superuser required>";
        m.board_serial = "<superuser required>";
    }

    m.board_vendor = utils::clean_dmi(utils::read_file_str(dmi_path + "board_vendor"));
    m.board_name = utils::clean_dmi(utils::read_file_str(dmi_path + "board_name"));
    m.board_version = utils::clean_dmi(utils::read_file_str(dmi_path + "board_version"));

    m.bios_vendor = utils::clean_dmi(utils::read_file_str(dmi_path + "bios_vendor"));
    m.bios_version = utils::clean_dmi(utils::read_file_str(dmi_path + "bios_version"));
    m.bios_date = utils::clean_dmi(utils::read_file_str(dmi_path + "bios_date"));

    std::string ctype = utils::read_file_str(dmi_path + "chassis_type");
    m.chassis_type = chassis_data(ctype);

    if (std::filesystem::exists("/sys/firmware/efi")) {
        m.uefi = "UEFI";
    } else {
        m.uefi = "BIOS";
    }
}

void MachineItem::dmi_data(MachineData& m) {}

MachineData MachineItem::machine_data() {
    MachineData m;
    sys_data(m);
    return m;
}

std::vector<OutputRow> MachineItem::short_output(const MachineData& m) {
    return full_output(m);
}

std::vector<OutputRow> MachineItem::full_output(const MachineData& m) {
    std::vector<OutputRow> rows;
    std::vector<Field> f1;
    f1.push_back(field("Type", m.chassis_type));
    f1.push_back(field("System", m.system_vendor));
    f1.push_back(field("product", m.product_name));
    f1.push_back(field("v", m.product_version));
    f1.push_back(field("serial", filter_data(m.product_serial)));
    rows.push_back(make_row("Machine", f1));

    std::vector<Field> f2;
    f2.push_back(field("Mobo", m.board_vendor));
    f2.push_back(field("model", m.board_name));
    if (!m.board_version.empty()) {
        f2.push_back(field("v", m.board_version));
    }
    f2.push_back(field("serial", filter_data(m.board_serial)));
    if (state_.extra >= 1) {
        f2.push_back(field("Firmware", m.uefi));
        f2.push_back(field("vendor", m.bios_vendor));
    } else {
        f2.push_back(field(m.uefi, m.bios_vendor));
    }
    f2.push_back(field("v", m.bios_version));
    f2.push_back(field("date", m.bios_date));
    rows.push_back(make_row("", f2, true));

    return rows;
}

std::vector<OutputRow> MachineItem::get() {
    auto data = machine_data();
    return full_output(data);
}

}  // namespace inxi
