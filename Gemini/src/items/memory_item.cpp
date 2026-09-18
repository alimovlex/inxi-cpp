// memory_item.cpp
#include "items/memory_item.h"
#include "utils/file_utils.h"
#include "utils/math_utils.h"
#include "utils/process_utils.h"
#include "utils/string_utils.h"
#include <sstream>

namespace inxi {

void MemoryItem::meminfo_data(MemorySummary& m) {
    auto lines = utils::read_file("/proc/meminfo");
    for (const auto& line : lines) {
        if (line.rfind("MemTotal:", 0) == 0) {
            m.total_bytes = std::stoull(utils::awk_field(line, 2)) * 1024ULL;
        } else if (line.rfind("MemFree:", 0) == 0) {
            m.free_bytes = std::stoull(utils::awk_field(line, 2)) * 1024ULL;
        } else if (line.rfind("MemAvailable:", 0) == 0) {
            m.avail_bytes = std::stoull(utils::awk_field(line, 2)) * 1024ULL;
        } else if (line.rfind("Buffers:", 0) == 0) {
            m.buffers_bytes = std::stoull(utils::awk_field(line, 2)) * 1024ULL;
        } else if (line.rfind("Cached:", 0) == 0) {
            m.cached_bytes = std::stoull(utils::awk_field(line, 2)) * 1024ULL;
        } else if (line.rfind("SwapTotal:", 0) == 0) {
            m.swap_total = std::stoull(utils::awk_field(line, 2)) * 1024ULL;
        } else if (line.rfind("SwapFree:", 0) == 0) {
            uint64_t sf = std::stoull(utils::awk_field(line, 2)) * 1024ULL;
            if (m.swap_total >= sf) m.swap_used = m.swap_total - sf;
        }
    }
    if (m.total_bytes >= m.avail_bytes) {
        m.used_bytes = m.total_bytes - m.avail_bytes;
    }
}

void MemoryItem::dmi_memory(MemorySummary& m) {}
void MemoryItem::sys_memory(MemorySummary& m) {}

MemorySummary MemoryItem::memory_data() {
    MemorySummary m;
    meminfo_data(m);
    dmi_memory(m);
    return m;
}

std::vector<OutputRow> MemoryItem::short_output(const MemorySummary& m) {
    std::vector<OutputRow> rows;
    std::vector<Field> fields;

    std::string total_h = utils::format_bytes(m.total_bytes, 2);
    std::string used_h = utils::format_bytes(m.used_bytes, 2);
    std::string pct = utils::format_percent(static_cast<double>(m.used_bytes), static_cast<double>(m.total_bytes));

    fields.push_back(field("RAM", "total: " + total_h + " used: " + used_h + " (" + pct + ")"));
    if (m.swap_total > 0) {
        std::string s_tot = utils::format_bytes(m.swap_total, 2);
        std::string s_used = utils::format_bytes(m.swap_used, 2);
        std::string s_pct = utils::format_percent(static_cast<double>(m.swap_used), static_cast<double>(m.swap_total));
        fields.push_back(field("Swap", "total: " + s_tot + " used: " + s_used + " (" + s_pct + ")"));
    }

    rows.push_back(make_row("Memory", fields));
    return rows;
}

std::vector<OutputRow> MemoryItem::full_output(const MemorySummary& m) {
    return short_output(m);
}

std::vector<OutputRow> MemoryItem::get() {
    auto m = memory_data();
    return full_output(m);
}

}  // namespace inxi
