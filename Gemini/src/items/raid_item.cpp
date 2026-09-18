// raid_item.cpp
#include "items/raid_item.h"
#include "utils/file_utils.h"
#include "utils/process_utils.h"
#include "utils/string_utils.h"

namespace inxi {

std::vector<RaidArray> RaidItem::mdstat_data() {
    std::vector<RaidArray> arrays;
    auto lines = utils::read_file("/proc/mdstat");
    for (size_t i = 0; i < lines.size(); ++i) {
        const auto& l = lines[i];
        if (l.rfind("md", 0) == 0) {
            RaidArray arr;
            arr.name = utils::awk_field(l, 1);
            arr.status = utils::awk_field(l, 2);
            arr.level = utils::awk_field(l, 3);
            arrays.push_back(arr);
        }
    }
    return arrays;
}

std::vector<RaidArray> RaidItem::lvm_raid_data() { return {}; }
std::vector<RaidArray> RaidItem::btrfs_raid_data() { return {}; }
std::vector<RaidArray> RaidItem::zfs_raid_data() { return {}; }

std::vector<OutputRow> RaidItem::full_output(const std::vector<RaidArray>& arrays) {
    std::vector<OutputRow> rows;
    if (arrays.empty()) {
        std::vector<Field> fields;
        fields.push_back(field("Message", "No RAID data was found."));
        rows.push_back(make_row("RAID", fields));
        return rows;
    }
    bool is_first = true;
    for (const auto& a : arrays) {
        std::vector<Field> fields;
        fields.push_back(field("Device-1", a.name));
        fields.push_back(field("type", a.level));
        fields.push_back(field("status", a.status));
        if (is_first) {
            rows.push_back(make_row("RAID", fields));
            is_first = false;
        } else {
            rows.push_back(make_row("", fields, true));
        }
    }
    return rows;
}

std::vector<OutputRow> RaidItem::short_output(const std::vector<RaidArray>& arrays) {
    return full_output(arrays);
}

std::vector<OutputRow> RaidItem::get() {
    auto arrays = mdstat_data();
    return full_output(arrays);
}

}  // namespace inxi
