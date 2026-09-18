// disk_item.h
// Collects block-device / hard-disk / SSD information.
// Perl: disk_data(), DiskItem-related subs throughout the codebase
//
// Perl subs mapped (spread across main/helpers):
//   lsblk_data()        → DiskItem::lsblk_data()
//   disk_data_bsd()     → DiskItem::disk_data_bsd()
//   disk_vendor()       → DiskItem::disk_vendor()
//   smart_data()        → DiskItem::smart_data()
//   transport_data()    → DiskItem::transport_data()
//   rotational_data()   → DiskItem::rotational_data()
//   get()               → DiskItem::get()
//   full_output()       → DiskItem::full_output()
//   short_output()      → DiskItem::short_output()
#pragma once
#include "items/info_item.h"
#include <string>
#include <vector>

namespace inxi {

struct DiskEntry {
    std::string name;       // e.g. "sda"
    std::string path;       // e.g. "/dev/sda"
    std::string model;
    std::string vendor;
    std::string serial;
    std::string firmware;
    std::string size;       // human-readable "500 GB"
    uint64_t    size_bytes = 0;
    std::string type;       // "HDD"|"SSD"|"NVMe"|"USB"|"Optical"|"Card"
    std::string transport;  // "SATA"|"NVMe"|"USB"|"SAS"|"SCSI"
    bool        rotational = true;
    int         rpm        = 0;
    std::string temp;       // e.g. "42 C"
    std::string smart;      // "Passed"|"Failed"|"N/A"
    std::string speed;      // interface speed
    std::string partition_scheme; // "GPT"|"MBR"|"Unknown"
};

class DiskItem final : public InfoItem {
public:
    explicit DiskItem(GlobalState& s) : InfoItem(s) {}
    std::string label() const override { return "Drives"; }
    std::vector<OutputRow> get() override;

private:
    // Perl: lsblk_data() — reads lsblk -o output
    std::vector<DiskEntry> lsblk_data();

    // Perl: disk_data_bsd() — reads BSD disk data (camcontrol/geom)
    std::vector<DiskEntry> disk_data_bsd();

    // Perl: disk_vendor() — resolves vendor from model or USB IDs
    std::string disk_vendor(const DiskEntry& d);

    // Perl: smart_data() — queries smartctl for health/temp
    void smart_data(DiskEntry& d);

    // Perl: transport_data() — determines interface transport
    std::string transport_data(const std::string& dev_path);

    // Perl: rotational_data() — reads /sys/.../queue/rotational
    bool rotational_data(const std::string& dev_name);

    // Perl: full_output() — detailed disk rows (-D -xx)
    std::vector<OutputRow> full_output(const std::vector<DiskEntry>& disks);

    // Perl: short_output() — brief disk rows (-D)
    std::vector<OutputRow> short_output(const std::vector<DiskEntry>& disks);
};

}  // namespace inxi
