// machine_item.h
// Collects system machine / BIOS / chassis / motherboard information.
// Perl: machine data helpers, dmidecode parsing
//
// Perl subs mapped:
//   get()              → MachineItem::get()
//   full_output()      → MachineItem::full_output()
//   short_output()     → MachineItem::short_output()
//   machine_data()     → MachineItem::machine_data()
//   dmi_data()         → MachineItem::dmi_data()
//   sys_data()         → MachineItem::sys_data()
//   chassis_data()     → MachineItem::chassis_data()
#pragma once
#include "items/info_item.h"
#include <string>
#include <vector>

namespace inxi {

struct MachineData {
    std::string system_vendor;
    std::string product_name;
    std::string product_version;
    std::string product_serial;
    std::string product_uuid;
    std::string board_vendor;
    std::string board_name;
    std::string board_version;
    std::string board_serial;
    std::string bios_vendor;
    std::string bios_version;
    std::string bios_date;
    std::string chassis_type;  // "Desktop"|"Notebook"|"Server"|…
    std::string chassis_vendor;
    std::string form_factor;
    bool        is_vm        = false;
    std::string vm_type;     // "KVM"|"VirtualBox"|"VMware"|…
    std::string uefi;        // "UEFI"|"BIOS"
};

class MachineItem final : public InfoItem {
public:
    explicit MachineItem(GlobalState& s) : InfoItem(s) {}
    std::string label() const override { return "Machine"; }
    std::vector<OutputRow> get() override;

private:
    std::vector<OutputRow> full_output(const MachineData& m);
    std::vector<OutputRow> short_output(const MachineData& m);

    MachineData machine_data();

    // Perl: reads /sys/class/dmi/id/* files
    void sys_data(MachineData& m);

    // Perl: parses dmidecode output
    void dmi_data(MachineData& m);

    // Perl: determines chassis type string
    std::string chassis_data(const std::string& raw_type);
};

}  // namespace inxi
