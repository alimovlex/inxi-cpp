// memory_item.h
// Collects RAM / memory slot information.
// Perl: memory-related subs, dmidecode type 17 parsing
//
// Perl subs mapped:
//   get()             → MemoryItem::get()
//   full_output()     → MemoryItem::full_output()
//   short_output()    → MemoryItem::short_output()
//   memory_data()     → MemoryItem::memory_data()
//   dmi_memory()      → MemoryItem::dmi_memory()
//   sys_memory()      → MemoryItem::sys_memory()
//   meminfo_data()    → MemoryItem::meminfo_data()
#pragma once
#include "items/info_item.h"
#include <string>
#include <vector>

namespace inxi {

struct MemorySlot {
    int         slot         = 0;
    std::string type;        // "DDR4"|"LPDDR5"|…
    std::string speed;       // "3200 MT/s"
    uint64_t    size_bytes   = 0;
    std::string size_human;  // "16 GiB"
    std::string manufacturer;
    std::string part_number;
    std::string serial;
    std::string locator;
    std::string bank_locator;
    bool        empty        = false;
    std::string ecc;         // "None"|"Single"|"Multi"
    std::string voltage;
};

struct MemorySummary {
    uint64_t    total_bytes   = 0;
    uint64_t    used_bytes    = 0;
    uint64_t    free_bytes    = 0;
    uint64_t    avail_bytes   = 0;
    uint64_t    buffers_bytes = 0;
    uint64_t    cached_bytes  = 0;
    uint64_t    swap_total    = 0;
    uint64_t    swap_used     = 0;
    int         num_slots     = 0;
    int         num_modules   = 0;
    std::string type;           // dominant type
    std::string speed;          // dominant speed
    std::vector<MemorySlot> slots;
};

class MemoryItem final : public InfoItem {
public:
    explicit MemoryItem(GlobalState& s) : InfoItem(s) {}
    std::string label() const override { return "Memory"; }
    std::vector<OutputRow> get() override;

private:
    std::vector<OutputRow> full_output(const MemorySummary& m);
    std::vector<OutputRow> short_output(const MemorySummary& m);

    MemorySummary memory_data();

    // Perl: dmidecode type 17 — physical slots
    void dmi_memory(MemorySummary& m);

    // Perl: /sys/devices/system/memory/ — size info on BSD/ARM
    void sys_memory(MemorySummary& m);

    // Perl: /proc/meminfo — usage data
    void meminfo_data(MemorySummary& m);
};

}  // namespace inxi
