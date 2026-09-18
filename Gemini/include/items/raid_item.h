// raid_item.h
// Collects software/hardware RAID information.
// Sources: md_raid, lvm_raid, btrfs_raid, zfs_raid, soft_raid in Perl
//
// Perl subs mapped:
//   get()               → RaidItem::get()
//   full_output()       → RaidItem::full_output()
//   short_output()      → RaidItem::short_output()
//   mdstat_data()       → RaidItem::mdstat_data()
//   lvm_raid_data()     → RaidItem::lvm_raid_data()
//   btrfs_raid_data()   → RaidItem::btrfs_raid_data()
//   zfs_raid_data()     → RaidItem::zfs_raid_data()
#pragma once
#include "items/info_item.h"
#include <string>
#include <vector>

namespace inxi {

struct RaidArray {
    std::string type;        // "md"|"lvm"|"btrfs"|"zfs"
    std::string name;        // "/dev/md0" or "md0"
    std::string level;       // "RAID0"|"RAID1"|"RAID5"|"RAID6"|"RAID10"
    std::string status;      // "active"|"degraded"|"resync"|"inactive"
    std::vector<std::string> members;
    int         size_mb      = 0;
    std::string size_human;
    int         chunk_kb     = 0;
    float       resync_pct   = 0.0f;
    std::string report;      // "UU_"|"[3/3][UUU]" etc.
    bool        bitmap       = false;
};

class RaidItem final : public InfoItem {
public:
    explicit RaidItem(GlobalState& s) : InfoItem(s) {}
    std::string label() const override { return "RAID"; }
    std::vector<OutputRow> get() override;

private:
    std::vector<OutputRow>  full_output(const std::vector<RaidArray>& arrays);
    std::vector<OutputRow>  short_output(const std::vector<RaidArray>& arrays);

    std::vector<RaidArray>  mdstat_data();
    std::vector<RaidArray>  lvm_raid_data();
    std::vector<RaidArray>  btrfs_raid_data();
    std::vector<RaidArray>  zfs_raid_data();
};

}  // namespace inxi
