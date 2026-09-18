// repo_item.cpp
#include "items/remaining_items.h"
#include "utils/file_utils.h"
#include "utils/string_utils.h"
#include <filesystem>

namespace inxi {

std::vector<OutputRow> RepoItem::get() {
    std::vector<OutputRow> rows;
    std::vector<Field> fields;

    // Void Linux xbps repos check
    auto xbps_files = utils::glob_files("/etc/xbps.d/*.conf");
    for (const auto& f : xbps_files) {
        auto lines = utils::read_file(f);
        for (const auto& l : lines) {
            if (l.rfind("repository=", 0) == 0) {
                fields.push_back(field("Active", l.substr(11)));
            }
        }
    }

    // Debian / Ubuntu apt repos check
    if (std::filesystem::exists("/etc/apt/sources.list")) {
        auto lines = utils::read_file("/etc/apt/sources.list");
        for (const auto& l : lines) {
            std::string t = utils::trim(l);
            if (t.rfind("deb ", 0) == 0) {
                fields.push_back(field("Active", t));
                break;
            }
        }
    }

    if (!fields.empty()) {
        rows.push_back(make_row("Repos", fields));
    }
    return rows;
}

}  // namespace inxi
