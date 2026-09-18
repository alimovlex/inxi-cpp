#pragma once

#include "inxi/options.hpp"

#include <string>
#include <vector>

namespace inxi {

struct Field {
    std::string key;
    std::string value;
};

struct Row {
    std::vector<Field> fields;
};

struct Section {
    std::string title;  // empty for short output
    std::vector<Row> rows;
};

struct Report {
    std::vector<Section> sections;
};

std::string render(const Report& report, const Options& opt);
void add_field(Row& row, std::string key, std::string value);
std::string json_escape(std::string_view s);
std::string to_json(const Report& report);
std::string to_xml(const Report& report);

}  // namespace inxi
