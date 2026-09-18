// partition_item.h
// Collects mounted-partition / filesystem information.
// Perl: partition_data(), PartitionItem subs
//
// Perl subs mapped:
//   get()                → PartitionItem::get()
//   full_output()        → PartitionItem::full_output()
//   short_output()       → PartitionItem::short_output()
//   partition_data()     → PartitionItem::partition_data()
//   proc_mounts_data()   → PartitionItem::proc_mounts_data()
//   df_data()            → PartitionItem::df_data()
//   sort_partitions()    → PartitionItem::sort_partitions()
#pragma once
#include "items/info_item.h"
#include <string>
#include <vector>

namespace inxi {

struct PartitionEntry {
    std::string dev;           // "/dev/sda1"
    std::string mountpoint;    // "/home"
    std::string fstype;        // "ext4"|"btrfs"|"xfs"…
    std::string label;
    std::string uuid;
    uint64_t    size_bytes  = 0;
    uint64_t    used_bytes  = 0;
    uint64_t    avail_bytes = 0;
    float       used_pct    = 0.0f;
    std::string size_human;
    std::string used_human;
    std::string avail_human;
    std::string options;       // mount options snippet
};

class PartitionItem final : public InfoItem {
public:
    explicit PartitionItem(GlobalState& s) : InfoItem(s) {}
    std::string label() const override { return "Partition"; }
    std::vector<OutputRow> get() override;

private:
    std::vector<OutputRow>    full_output(const std::vector<PartitionEntry>& parts);
    std::vector<OutputRow>    short_output(const std::vector<PartitionEntry>& parts);

    std::vector<PartitionEntry> partition_data();
    std::vector<PartitionEntry> proc_mounts_data();
    void                        df_data(std::vector<PartitionEntry>& parts);

    // Perl: partition-sort by id|size|used|label|uuid|mount
    void sort_partitions(std::vector<PartitionEntry>& parts,
                         const std::string& sort_key);
};

}  // namespace inxi
