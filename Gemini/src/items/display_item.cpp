// display_item.cpp
#include "items/remaining_items.h"
#include "utils/file_utils.h"
#include "utils/string_utils.h"

namespace inxi {

std::vector<OutputRow> DisplayItem::get() {
    std::vector<OutputRow> rows;
    auto mode_files = utils::glob_files("/sys/class/drm/card*-*/modes");
    std::vector<Field> fields;
    int id = 1;
    for (const auto& mf : mode_files) {
        std::string mode = utils::read_file_str(mf);
        if (!mode.empty()) {
            fields.push_back(field("Display-" + std::to_string(id++), utils::trim(utils::split_lines(mode)[0])));
        }
    }
    if (!fields.empty()) {
        rows.push_back(make_row("Display", fields));
    }
    return rows;
}

}  // namespace inxi
