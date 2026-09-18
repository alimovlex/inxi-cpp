// output.cpp
#include "output.h"
#include "utils/string_utils.h"
#include <iostream>
#include <sstream>

#ifdef HAVE_NLOHMANN_JSON
#include <nlohmann/json.hpp>
#endif

namespace inxi {

Output::Output(GlobalState& state) : state_(state) {}

std::string Output::apply_colors(const std::string& s, const std::string& role) {
    if (state_.colors.scheme == 0) return s;
    if (role == "c1") return state_.colors.c1 + s + state_.colors.reset;
    if (role == "c2") return state_.colors.c2 + s + state_.colors.reset;
    if (role == "c3") return state_.colors.c3 + s + state_.colors.reset;
    return s;
}

std::string Output::key(const std::string& k) {
    return apply_colors(k + state_.sep.s1, "c2");
}

std::string Output::get_chip_id(const std::string& raw_id) {
    return raw_id;
}

void Output::increment_starters() {
    starter_count_++;
}

std::string Output::make_line(const OutputRow& row) {
    std::string line;
    if (!row.label.empty() && !row.is_continuation) {
        line += apply_colors(row.label + state_.sep.s1, "c1");
    } else if (row.is_continuation) {
        line += "  ";
    }

    bool first = true;
    for (const auto& f : row.fields) {
        if (!line.empty() && !first) {
            line += " ";
        } else if (!row.label.empty() && first && !row.is_continuation) {
            // Label is on its own line if it's a major section in inxi -b
            // But if it's a short row (e.g. CPU: ...), inline it
            line += " ";
        }
        first = false;

        if (!f.key.empty()) {
            line += key(f.key) + " " + f.value;
        } else {
            line += f.value;
        }
    }
    return line;
}

std::string Output::wrap_line(const std::string& s, int max_width, int indent) {
    if (static_cast<int>(utils::visible_length(s)) <= max_width) return s;

    // Detect leading indentation of s (e.g. 2 spaces)
    size_t lead_spaces = 0;
    while (lead_spaces < s.size() && s[lead_spaces] == ' ') {
        lead_spaces++;
    }
    std::string first_indent(lead_spaces, ' ');
    std::string indent_str(indent, ' ');

    std::istringstream iss(s);
    std::string word;
    std::string result;
    std::string current_line;
    int current_visible_len = 0;

    while (iss >> word) {
        int word_visible_len = static_cast<int>(utils::visible_length(word));
        if (current_line.empty()) {
            current_line = first_indent + word;
            current_visible_len = static_cast<int>(lead_spaces) + word_visible_len;
        } else if (current_visible_len + 1 + word_visible_len <= max_width) {
            current_line += " " + word;
            current_visible_len += 1 + word_visible_len;
        } else {
            if (!result.empty()) result += "\n";
            result += current_line;
            current_line = indent_str + word;
            current_visible_len = indent + word_visible_len;
        }
    }
    if (!current_line.empty()) {
        if (!result.empty()) result += "\n";
        result += current_line;
    }
    return result;
}

void Output::print_line(const std::string& line) {
    std::cout << line << "\n";
}

void Output::print_basic(const std::string& text) {
    std::cout << text << "\n";
}

void Output::print_data(const std::vector<OutputRow>& rows) {
    for (const auto& row : rows) {
        if (row.fields.empty() && !row.label.empty()) {
            print_line(apply_colors(row.label + state_.sep.s1, "c1"));
        } else if (!row.label.empty() && !row.is_continuation) {
            print_line(apply_colors(row.label + state_.sep.s1, "c1"));
            std::string line = "  ";
            bool first = true;
            for (const auto& f : row.fields) {
                if (!first) line += " ";
                first = false;
                if (!f.key.empty()) {
                    if (f.value.empty()) {
                        line += key(f.key);
                    } else {
                        line += key(f.key) + " " + f.value;
                    }
                } else {
                    line += f.value;
                }
            }
            print_line(wrap_line(line, state_.size.max_wrap, 4));
        } else {
            std::string line = "  ";
            bool first = true;
            for (const auto& f : row.fields) {
                if (!first) line += " ";
                first = false;
                if (!f.key.empty()) {
                    if (f.value.empty()) {
                        line += key(f.key);
                    } else {
                        line += key(f.key) + " " + f.value;
                    }
                } else {
                    line += f.value;
                }
            }
            print_line(wrap_line(line, state_.size.max_wrap, 4));
        }
    }
}

std::string Output::generate_json(const std::vector<OutputRow>& rows) {
#ifdef HAVE_NLOHMANN_JSON
    nlohmann::json j;
    for (const auto& row : rows) {
        nlohmann::json row_obj;
        for (const auto& f : row.fields) {
            row_obj[f.key] = f.value;
        }
        if (!row.label.empty()) {
            j[row.label].push_back(row_obj);
        } else {
            j["rows"].push_back(row_obj);
        }
    }
    return j.dump(2);
#else
    return "{}";
#endif
}

std::string Output::generate_xml(const std::vector<OutputRow>& rows) {
    (void)rows;
    return "<inxi></inxi>";
}

void Output::control(const std::vector<OutputRow>& rows) {
    print_data(rows);
}

void Output::handle(const std::vector<OutputRow>& rows) {
    if (state_.output_type == "json") {
        print_line(generate_json(rows));
    } else if (state_.output_type == "xml") {
        print_line(generate_xml(rows));
    } else {
        control(rows);
    }
}

}  // namespace inxi
