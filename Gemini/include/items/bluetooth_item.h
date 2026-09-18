// bluetooth_item.h
// Collects Bluetooth adapter and device information.
// Perl source: package BluetoothItem (lines ~8805–9372)
//
// Perl subs mapped:
//   get()                 → BluetoothItem::get()
//   device_output()       → BluetoothItem::device_output()
//   usb_output()          → BluetoothItem::usb_output()
//   advanced_output()     → BluetoothItem::advanced_output()
//   set_bluetooth_data()  → BluetoothItem::set_bluetooth_data()
//   bt_adapter_data()     → BluetoothItem::bt_adapter_data()
//   btmgmt_data()         → BluetoothItem::btmgmt_data()
//   hciconfig_data()      → BluetoothItem::hciconfig_data()
//   rfkill_data()         → BluetoothItem::rfkill_data()
//   check_service()       → BluetoothItem::check_service()
//   bluetooth_version()   → BluetoothItem::bluetooth_version()
#pragma once
#include "items/info_item.h"
#include <string>
#include <vector>

namespace inxi {

struct BluetoothAdapter {
    std::string id;
    std::string name;
    std::string driver;
    std::string vendor;
    std::string product;
    std::string version;
    std::string firmware;
    std::string mac;
    bool        rfkill_blocked = false;
    std::string service_status; // "active"|"inactive"|""
    std::string chip_id;        // USB chip id
    bool        is_usb = false;
    std::string bus_id;
    std::string type;
    std::string state;
    std::string report_tool;
    std::string hci_id;
};

class BluetoothItem final : public InfoItem {
public:
    explicit BluetoothItem(GlobalState& s) : InfoItem(s) {}
    std::string label() const override { return "Bluetooth"; }
    std::vector<OutputRow> get() override;

private:
    // Perl: device_output()
    std::vector<OutputRow> device_output(const std::vector<BluetoothAdapter>& adapters);

    // Perl: usb_output()
    std::vector<OutputRow> usb_output(const std::vector<BluetoothAdapter>& adapters);

    // Perl: advanced_output()
    std::vector<OutputRow> advanced_output(const std::vector<BluetoothAdapter>& adapters);

    // Perl: set_bluetooth_data() — top-level data orchestrator
    std::vector<BluetoothAdapter> set_bluetooth_data();

    // Perl: bt_adapter_data() — queries bluetoothctl
    std::vector<BluetoothAdapter> bt_adapter_data();

    // Perl: btmgmt_data() — queries btmgmt
    std::vector<BluetoothAdapter> btmgmt_data();

    // Perl: hciconfig_data() — queries hciconfig
    std::vector<BluetoothAdapter> hciconfig_data();

    // Perl: rfkill_data() — queries rfkill for soft/hard block state
    void rfkill_data(std::vector<BluetoothAdapter>& adapters);

    // Perl: check_service() — checks systemd/openrc BT service status
    std::string check_service();

    // Perl: bluetooth_version() — extracts BT version from hciconfig output
    std::string bluetooth_version(const std::string& raw);
};

}  // namespace inxi
