#include "inxi/output.hpp"

#include "inxi/util.hpp"

#include <cstdio>
#include <iostream>
#include <sstream>

namespace inxi {

void add_field(Row& row, std::string key, std::string value) {
    if (value.empty()) return;
    row.fields.push_back(Field{std::move(key), std::move(value)});
}

std::string json_escape(std::string_view s) {
    std::string o;
    o.reserve(s.size() + 8);
    for (unsigned char c : s) {
        switch (c) {
            case '"': o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n"; break;
            case '\t': o += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    o += buf;
                } else {
                    o.push_back(static_cast<char>(c));
                }
        }
    }
    return o;
}

std::string to_json(const Report& report) {
    std::ostringstream ss;
    ss << "{\n";
    for (size_t si = 0; si < report.sections.size(); ++si) {
        auto& sec = report.sections[si];
        std::string title = sec.title.empty() ? "short" : sec.title;
        ss << "  \"" << json_escape(title) << "\": [\n";
        for (size_t ri = 0; ri < sec.rows.size(); ++ri) {
            ss << "    {";
            auto& row = sec.rows[ri];
            for (size_t fi = 0; fi < row.fields.size(); ++fi) {
                if (fi) ss << ", ";
                ss << "\"" << json_escape(row.fields[fi].key) << "\": \""
                   << json_escape(row.fields[fi].value) << "\"";
            }
            ss << "}";
            if (ri + 1 < sec.rows.size()) ss << ",";
            ss << "\n";
        }
        ss << "  ]";
        if (si + 1 < report.sections.size()) ss << ",";
        ss << "\n";
    }
    ss << "}\n";
    return ss.str();
}

std::string to_xml(const Report& report) {
    std::ostringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<inxi>\n";
    for (auto& sec : report.sections) {
        std::string title = sec.title.empty() ? "short" : sec.title;
        ss << "  <section name=\"" << json_escape(title) << "\">\n";
        for (auto& row : sec.rows) {
            ss << "    <row";
            for (auto& f : row.fields) {
                ss << " " << f.key << "=\"" << json_escape(f.value) << "\"";
            }
            ss << "/>\n";
        }
        ss << "  </section>\n";
    }
    ss << "</inxi>\n";
    return ss.str();
}

static std::string color_on(const Options& opt) {
    bool use = opt.color != 0 && (opt.color > 0 || stdout_is_tty());
    if (opt.output != "screen") use = false;
    if (!use) return {};
    return "\033[1;36m";  // bold cyan labels, similar to inxi scheme 12
}

static std::string color_off(const Options& opt) {
    if (color_on(opt).empty()) return {};
    return "\033[0m";
}

static int visible_len(std::string_view s) {
    int n = 0;
    bool esc = false;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\033') {
            esc = true;
            continue;
        }
        if (esc) {
            if (s[i] == 'm') esc = false;
            continue;
        }
        ++n;
    }
    return n;
}

std::string render(const Report& report, const Options& opt) {
    if (opt.output == "json") return to_json(report);
    if (opt.output == "xml") return to_xml(report);

    const int width = opt.width > 0 ? opt.width : term_width();
    const std::string on = color_on(opt);
    const std::string off = color_off(opt);
    const std::string sep = opt.separator.empty() ? ": " : opt.separator;

    std::ostringstream out;

    auto emit_wrapped = [&](const std::string& indent, const Row& row) {
        std::string line = indent;
        int col = visible_len(indent);
        for (size_t i = 0; i < row.fields.size(); ++i) {
            std::string piece;
            if (i) piece += " ";
            piece += on + row.fields[i].key + off + sep + row.fields[i].value;
            int plen = visible_len(piece);
            if (col > visible_len(indent) && col + plen > width && width >= 60) {
                out << line << '\n';
                line = indent + "  ";
                col = visible_len(line);
                piece = on + row.fields[i].key + off + sep + row.fields[i].value;
                plen = visible_len(piece);
            }
            line += piece;
            col += plen;
        }
        if (!line.empty() && line != indent) out << line << '\n';
    };

    for (auto& sec : report.sections) {
        if (!sec.title.empty()) {
            out << on << sec.title << off << sep << "\n";
        }
        std::string indent = sec.title.empty() ? "" : "  ";
        for (auto& row : sec.rows) emit_wrapped(indent, row);
    }
    return out.str();
}

}  // namespace inxi
