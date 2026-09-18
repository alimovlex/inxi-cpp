// usb_item.cpp
#include "items/remaining_items.h"
#include "utils/process_utils.h"
#include "utils/string_utils.h"

namespace inxi {

std::vector<OutputRow> UsbItem::get() {
    std::vector<OutputRow> rows;
    auto lines = utils::run_capture("lsusb");
    int id = 1;
    bool is_first = true;
    for (const auto& l : lines) {
        size_t id_pos = l.find("ID ");
        if (id_pos != std::string::npos && l.size() > id_pos + 13) {
            std::string usb_id = l.substr(id_pos + 3, 9);
            std::string desc = utils::trim(l.substr(id_pos + 13));
            std::vector<Field> fields;
            fields.push_back(field("Hub-" + std::to_string(id++), desc));
            fields.push_back(field("chip-ID", usb_id));
            if (is_first) {
                rows.push_back(make_row("USB", fields));
                is_first = false;
            } else {
                rows.push_back(make_row("", fields, true));
            }
        }
    }
    return rows;
}

}  // namespace inxi
