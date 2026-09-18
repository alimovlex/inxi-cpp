#pragma once

#include "inxi/options.hpp"
#include "inxi/output.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace inxi {

void collect_short(Report& r, const Options& o);
void collect_cpu(Report& r, const Options& o, bool basic);
void collect_system(Report& r, const Options& o);
void collect_info(Report& r, const Options& o);
void collect_machine(Report& r, const Options& o);
void collect_battery(Report& r, const Options& o);
void collect_graphics(Report& r, const Options& o);
void collect_audio(Report& r, const Options& o);
void collect_network(Report& r, const Options& o);
void collect_bluetooth(Report& r, const Options& o);
void collect_usb(Report& r, const Options& o);
void collect_drives(Report& r, const Options& o);
void collect_partitions(Report& r, const Options& o);
void collect_swap(Report& r, const Options& o);
void collect_unmounted(Report& r, const Options& o);
void collect_raid(Report& r, const Options& o);
void collect_logical(Report& r, const Options& o);
void collect_sensors(Report& r, const Options& o);
void collect_repos(Report& r, const Options& o);
void collect_ram(Report& r, const Options& o);
void collect_processes(Report& r, const Options& o);
void collect_slots(Report& r, const Options& o);
void collect_weather(Report& r, const Options& o);

struct CpuShort {
    std::string model;
    std::string type;
    std::string cores_word;
    unsigned sockets = 1;
    unsigned cores = 1;
    unsigned threads = 1;
    double speed = 0;
    double min = 0;
    double max = 0;
    std::string flags;
};
CpuShort cpu_summary();

struct MemShort {
    double total_kib = 0;
    double available_kib = 0;
    double used_kib = 0;
};
MemShort mem_summary();

struct DiskShort {
    std::uint64_t total_bytes = 0;
    std::uint64_t used_bytes = 0;
};
DiskShort disk_summary();

}  // namespace inxi
