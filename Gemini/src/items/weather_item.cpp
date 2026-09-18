// weather_item.cpp
#include "items/remaining_items.h"
#include "downloader.h"
#include "utils/string_utils.h"

namespace inxi {

std::vector<OutputRow> WeatherItem::get() {
    std::vector<OutputRow> rows;
    Downloader dl(state_);
    // Query wttr.in format=3 for a 1-line weather report
    std::string report = utils::trim(dl.download_string("https://wttr.in/?format=3"));
    if (!report.empty()) {
        std::vector<Field> fields;
        fields.push_back(field("Report", report));
        rows.push_back(make_row("Weather", fields));
    }
    return rows;
}

}  // namespace inxi
