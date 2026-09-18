// swap_item.cpp
#include "items/remaining_items.h"
#include "utils/file_utils.h"
#include "utils/math_utils.h"
#include "utils/process_utils.h"
#include "utils/string_utils.h"
#include <filesystem>

namespace inxi {

std::vector<OutputRow> SwapItem::get() {
    std::vector<OutputRow> rows;
    bool is_first = true;

    if (state_.extra >= 1) {
        std::string swappiness = utils::trim(utils::read_file_str("/proc/sys/vm/swappiness"));
        std::string cache_pressure = utils::trim(utils::read_file_str("/proc/sys/vm/vfs_cache_pressure"));
        if (!swappiness.empty()) {
            std::vector<Field> k_fields;
            k_fields.push_back(field("Kernel", ""));
            k_fields.push_back(field("swappiness", swappiness + " (default)"));
            if (!cache_pressure.empty()) {
                k_fields.push_back(field("cache-pressure", cache_pressure + " (default)"));
            }
            k_fields.push_back(field("zswap", "no"));
            rows.push_back(make_row("Swap", k_fields));
            is_first = false;
        }
    }

    auto lines = utils::read_file("/proc/swaps");
    // Format: Filename Type Size Used Priority
    int id = 1;
    for (size_t i = 1; i < lines.size(); ++i) {
        std::string fname = utils::awk_field(lines[i], 1);
        std::string type = utils::awk_field(lines[i], 2);
        std::string size_kb = utils::awk_field(lines[i], 3);
        std::string used_kb = utils::awk_field(lines[i], 4);
        std::string prio = utils::awk_field(lines[i], 5);

        if (!size_kb.empty() && utils::is_int(size_kb)) {
            uint64_t size_b = std::stoull(size_kb) * 1024ULL;
            uint64_t used_b = utils::is_int(used_kb) ? std::stoull(used_kb) * 1024ULL : 0;
            std::vector<Field> fields;
            std::string id_key = "ID-" + std::to_string(id);
            std::string id_val = "swap-" + std::to_string(id);
            id++;
            fields.push_back(field(id_key, id_val));
            fields.push_back(field("type", type));
            fields.push_back(field("size", utils::format_bytes(size_b, 2)));
            fields.push_back(field("used", utils::format_bytes(used_b, 2) + " (" + utils::format_percent(used_b, size_b) + ")"));
            fields.push_back(field("priority", prio));
            fields.push_back(field("dev", fname));

            // maj-min from /sys/class/block
            if (state_.extra >= 1 && fname.rfind("/dev/", 0) == 0) {
                std::string dev_base = std::filesystem::path(fname).filename().string();
                std::string dev_file = "/sys/class/block/" + dev_base + "/dev";
                std::string majmin = utils::trim(utils::read_file_str(dev_file));
                if (!majmin.empty()) {
                    fields.push_back(field("maj-min", majmin));
                }
            }

            if (is_first) {
                rows.push_back(make_row("Swap", fields));
                is_first = false;
            } else {
                rows.push_back(make_row("", fields, true));
            }
        }
    }
    return rows;
}

}  // namespace inxi
