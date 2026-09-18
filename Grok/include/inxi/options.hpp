#pragma once

#include <string>
#include <unordered_map>

namespace inxi {

struct Options {
    bool show_short = true;
    bool show_system = false;
    bool show_machine = false;
    bool show_battery = false;
    bool show_battery_forced = false;
    bool show_cpu = false;
    bool show_cpu_basic = false;
    bool show_cpu_flag = false;
    bool show_graphic = false;
    bool show_graphic_full = false;
    bool show_audio = false;
    bool show_network = false;
    bool show_network_advanced = false;
    bool show_ip = false;
    bool show_bluetooth = false;
    bool show_bluetooth_forced = false;
    bool show_disk = false;
    bool show_disk_basic = false;
    bool show_disk_total = false;
    bool show_optical = false;
    bool show_partition = false;
    bool show_partition_full = false;
    bool show_swap = false;
    bool show_unmounted = false;
    bool show_usb = false;
    bool show_raid = false;
    bool show_raid_basic = false;
    bool show_logical = false;
    bool show_ram = false;
    bool show_ram_modules = false;
    bool show_ram_short = false;
    bool show_sensor = false;
    bool show_repo = false;
    bool show_process = false;
    bool show_ps_cpu = false;
    bool show_ps_mem = false;
    bool show_slot = false;
    bool show_info = false;
    bool show_weather = false;
    bool show_label = false;
    bool show_uuid = false;
    bool show_help = false;
    bool show_version = false;
    bool show_version_short = false;
    bool show_recommends = false;
    bool show_configs = false;

    int extra = 0;   // -x / -xx / -xxx
    bool admin = false;
    bool filter = false;
    bool filter_override = false;
    int color = -1;  // -1 auto, 0 off
    int width = 0;   // 0 = detect
    int ps_count = 5;
    std::string weather_location;
    std::string weather_unit = "m";
    std::string output = "screen";  // screen|json|xml
    std::string separator;
    std::string weather_source;

    std::unordered_map<std::string, bool> force;
};

Options parse_args(int argc, char** argv);
void apply_verbosity(Options& o, int level);
void apply_basic(Options& o);
void apply_expanded(Options& o);
void print_help();
void print_version(bool short_form);

}  // namespace inxi
