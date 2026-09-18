// info_item.h
// Abstract base class for all hardware/software info collectors.
// Every "Item" in Perl (AudioItem, CpuItem, DiskItem, …) maps to a
// concrete subclass of InfoItem.
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "global_state.h"

namespace inxi {

// A single key/value pair in a row of output
struct Field {
    std::string key;
    std::string value;
};

// One logical line of output (label + fields)
struct OutputRow {
    std::string label;              // left-hand side label (e.g. "CPU")
    std::vector<Field> fields;      // ordered list of key/value pairs
    bool is_continuation = false;   // true → indented continuation line
};

// -----------------------------------------------------------------------
// InfoItem — abstract base
// -----------------------------------------------------------------------
class InfoItem {
public:
    explicit InfoItem(GlobalState& state) : state_(state) {}
    virtual ~InfoItem() = default;

    // Collect data and return formatted output rows.
    // Perl: <ClassName>::get()
    virtual std::vector<OutputRow> get() = 0;

    // Return the section label (e.g. "Audio", "CPU")
    virtual std::string label() const = 0;

protected:
    GlobalState& state_;

    // Helper: append a new continuation row
    static OutputRow make_row(const std::string& label,
                              std::vector<Field> fields,
                              bool continuation = false) {
        return OutputRow{label, std::move(fields), continuation};
    }

    // Helper: make a simple key→value field
    static Field field(const std::string& k, const std::string& v) {
        return Field{k, v};
    }

    // Helper: filter sensitive strings (MAC, serial, IP) when -z is active
    std::string filter_data(const std::string& s) const {
        if (state_.filter) {
            if (s.empty() || s == "<superuser required>") return s;
            return "<filter>";
        }
        return s;
    }
};

// Concrete collector for the "Info:" section (Processes, Uptime, Memory, Shell, inxi version)
class GeneralInfoItem final : public InfoItem {
public:
    explicit GeneralInfoItem(GlobalState& s) : InfoItem(s) {}
    std::string label() const override { return "Info"; }
    std::vector<OutputRow> get() override;
};

}  // namespace inxi

