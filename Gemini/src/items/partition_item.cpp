// partition_item.cpp
#include "items/partition_item.h"
#include "utils/file_utils.h"
#include "utils/math_utils.h"
#include "utils/process_utils.h"
#include "utils/string_utils.h"
#include <algorithm>
#include <filesystem>
#include <sys/statvfs.h>

namespace inxi {

std::vector<PartitionEntry> PartitionItem::proc_mounts_data() {
    std::vector<PartitionEntry> entries;
    auto lines = utils::read_file("/proc/mounts");
    for (const auto& line : lines) {
        std::string dev = utils::awk_field(line, 1);
        std::string mnt = utils::awk_field(line, 2);
        std::string fstype = utils::awk_field(line, 3);
        if (dev.rfind("/dev/", 0) == 0 && fstype != "devtmpfs" && fstype != "tmpfs") {
            PartitionEntry e;
            e.dev = dev;
            e.mountpoint = mnt;
            e.fstype = fstype;
            entries.push_back(e);
        }
    }
    return entries;
}

void PartitionItem::df_data(std::vector<PartitionEntry>& parts) {
    for (auto& p : parts) {
        struct statvfs st;
        if (statvfs(p.mountpoint.c_str(), &st) == 0) {
            uint64_t bsize = st.f_frsize ? st.f_frsize : st.f_bsize;
            p.size_bytes = st.f_blocks * bsize;
            p.avail_bytes = st.f_bavail * bsize;
            if (p.size_bytes >= p.avail_bytes) {
                p.used_bytes = p.size_bytes - p.avail_bytes;
            }
            if (p.size_bytes > 0) {
                p.used_pct = static_cast<float>((static_cast<double>(p.used_bytes) / p.size_bytes) * 100.0);
            }
            p.size_human = utils::format_bytes(p.size_bytes, 2);
            p.used_human = utils::format_bytes(p.used_bytes, 2);
            p.avail_human = utils::format_bytes(p.avail_bytes, 2);
        }
    }
}

void PartitionItem::sort_partitions(std::vector<PartitionEntry>& parts,
                                     const std::string& /*sort_key*/) {
    std::sort(parts.begin(), parts.end(), [](const PartitionEntry& a, const PartitionEntry& b) {
        return a.mountpoint < b.mountpoint;
    });
}

std::vector<PartitionEntry> PartitionItem::partition_data() {
    auto parts = proc_mounts_data();
    df_data(parts);
    sort_partitions(parts, state_.show.partition_sort);
    return parts;
}

std::vector<OutputRow> PartitionItem::full_output(const std::vector<PartitionEntry>& parts) {
    std::vector<OutputRow> rows;
    int id = 1;
    bool is_first = true;
    for (const auto& p : parts) {
        std::vector<Field> fields;
        fields.push_back(field("ID-" + std::to_string(id++), p.mountpoint));

        // raw-size: the full block-device size reported by lsblk (or same as size)
        // We approximate raw-size from statvfs total (same value, shown differently)
        fields.push_back(field("raw-size", p.size_human));
        // percentage used of raw size
        std::string pct = utils::format_percent(p.used_bytes, p.size_bytes);
        fields.push_back(field("size", p.size_human + " (100.00%)"));
        fields.push_back(field("used", p.used_human + " (" + pct + ")"));
        fields.push_back(field("fs", p.fstype));
        fields.push_back(field("dev", p.dev));

        // maj-min from /sys/class/block
        if (state_.extra >= 1) {
            std::string dev_base = std::filesystem::path(p.dev).filename().string();
            std::string dev_file = "/sys/class/block/" + dev_base + "/dev";
            std::string majmin = utils::trim(utils::read_file_str(dev_file));
            if (!majmin.empty()) {
                fields.push_back(field("maj-min", majmin));
            }
        }

        if (is_first) {
            rows.push_back(make_row("Partition", fields));
            is_first = false;
        } else {
            rows.push_back(make_row("", fields, true));
        }
    }
    return rows;
}

std::vector<OutputRow> PartitionItem::short_output(const std::vector<PartitionEntry>& parts) {
    return full_output(parts);
}

std::vector<OutputRow> PartitionItem::get() {
    auto parts = partition_data();
    return full_output(parts);
}

}  // namespace inxi
