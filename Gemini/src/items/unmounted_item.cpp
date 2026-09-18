// unmounted_item.cpp
#include "items/remaining_items.h"
#include "utils/file_utils.h"
#include "utils/math_utils.h"
#include "utils/process_utils.h"
#include "utils/string_utils.h"
#include <set>

namespace inxi {

std::vector<OutputRow> UnmountedItem::get() {
    std::vector<OutputRow> rows;
    // Collect mounted devices
    std::set<std::string> mounted;
    auto mount_lines = utils::read_file("/proc/mounts");
    for (const auto& l : mount_lines) {
        std::string dev = utils::awk_field(l, 1);
        if (dev.rfind("/dev/", 0) == 0) {
            mounted.insert(dev.substr(5));
        }
    }

    // Collect all partition devices from /proc/partitions
    auto part_lines = utils::read_file("/proc/partitions");
    std::vector<Field> fields;
    int id = 1;
    for (size_t i = 2; i < part_lines.size(); ++i) {
        std::string name = utils::awk_field(part_lines[i], 4);
        std::string blocks = utils::awk_field(part_lines[i], 3);
        if (name.empty() || mounted.count(name) > 0) continue;
        // Check if it's a partition (ends in a digit)
        if (std::isdigit(name.back()) && utils::is_int(blocks)) {
            uint64_t bytes = std::stoull(blocks) * 1024ULL;
            fields.push_back(field("ID-" + std::to_string(id++), "/dev/" + name + " size: " + utils::format_bytes(bytes, 2)));
        }
    }

    if (!fields.empty()) {
        rows.push_back(make_row("Unmounted", fields));
    }
    return rows;
}

}  // namespace inxi
