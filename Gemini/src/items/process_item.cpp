// process_item.cpp
#include "items/remaining_items.h"
#include "utils/process_utils.h"
#include "utils/string_utils.h"

namespace inxi {

std::vector<OutputRow> ProcessItem::get() {
    std::vector<OutputRow> rows;
    // Show top processes if requested
    auto lines = utils::run_capture("ps -eo pid,%cpu,%mem,comm --sort=-%cpu | head -n 6");
    if (lines.size() > 1) {
        std::vector<Field> fields;
        for (size_t i = 1; i < lines.size(); ++i) {
            std::string comm = utils::awk_field(lines[i], 4);
            std::string cpu = utils::awk_field(lines[i], 2);
            if (!comm.empty()) {
                fields.push_back(field(comm, cpu + "% cpu"));
            }
        }
        rows.push_back(make_row("Processes", fields));
    }
    return rows;
}

}  // namespace inxi
