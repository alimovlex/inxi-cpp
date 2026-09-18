// cpu_item.h
// Collects CPU / processor information.
// Perl source: package CpuItem (lines ~9374–10281+)
//
// Perl subs mapped:
//   get()                   → CpuItem::get()
//   full_output()           → CpuItem::full_output()
//   full_output_caches()    → CpuItem::full_output_caches()
//   short_output()          → CpuItem::short_output()
//   short_data()            → CpuItem::short_data()
//   prep_short_data()       → CpuItem::prep_short_data()
//   cpuinfo_data()          → CpuItem::cpuinfo_data()
//   cpuinfo_data_grabber()  → CpuItem::cpuinfo_data_grabber()
//   cpu_sys_data()          → CpuItem::cpu_sys_data()
//   (clean_speed)           → CpuItem::clean_speed()  [static helper]
#pragma once
#include "items/info_item.h"
#include <string>
#include <vector>
#include <map>

namespace inxi {

// Per-CPU-core topology + frequency data
struct CpuCore {
    int    physical_id  = 0;
    int    core_id      = 0;
    int    cpu_id       = 0;
    int    cur_freq_mhz = 0;
    int    max_freq_mhz = 0;
    int    min_freq_mhz = 0;
    std::string scaling_governor;
    std::string scaling_driver;
};

// Per-physical-CPU data
struct CpuPhysical {
    int    id             = 0;
    std::string model;
    std::string vendor;
    std::string family;
    std::string stepping;
    int    cores          = 0;  // physical cores
    int    threads        = 0;  // logical threads
    int    max_freq_mhz   = 0;
    int    min_freq_mhz   = 0;
    bool   boost          = false;
    std::vector<std::string> governors;
    std::map<std::string, std::string> caches; // "l1d","l1i","l2","l3" → size str
    std::vector<std::string> flags;
    std::vector<std::string> vulnerabilities;
    std::vector<CpuCore> logical_cores;
};

// Full aggregated CPU result
struct CpuData {
    std::vector<CpuPhysical> cpus;
    bool   has_boost     = false;
    std::string arch;           // e.g. "x86_64", "aarch64"
    int    bits          = 0;
    std::string bogomips;
};

class CpuItem final : public InfoItem {
public:
    explicit CpuItem(GlobalState& s) : InfoItem(s) {}
    std::string label() const override { return "CPU"; }
    std::vector<OutputRow> get() override;

private:
    // Perl: full_output() — extended CPU output (-C -xx)
    std::vector<OutputRow> full_output(const CpuData& data);

    // Perl: full_output_caches() — cache lines within full_output
    std::vector<OutputRow> full_output_caches(const CpuPhysical& cpu);

    // Perl: short_output() — brief one-line CPU output (-C)
    std::vector<OutputRow> short_output(const CpuData& data);

    // Perl: short_data() — computes brief CPU summary from CpuData
    std::string short_data(const CpuData& data);

    // Perl: prep_short_data() — normalises model name strings
    std::string prep_short_data(const std::string& model);

    // Perl: cpuinfo_data() — full parse of /proc/cpuinfo
    CpuData cpuinfo_data();

    // Perl: cpuinfo_data_grabber() — raw line-by-line /proc/cpuinfo reader
    std::vector<std::string> cpuinfo_data_grabber();

    // Perl: cpu_sys_data() — reads /sys/devices/system/cpu/ hierarchy
    CpuData cpu_sys_data(const std::map<int,int>* sys_freq = nullptr);

    // Helper: convert kHz string to MHz int
    static int clean_speed(const std::string& khz_str);
};

}  // namespace inxi
