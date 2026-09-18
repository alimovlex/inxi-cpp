#include "inxi/options.hpp"

#include "inxi/util.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace inxi {
namespace {

void disable_short(Options& o) { o.show_short = false; }

bool is_digits(const std::string& s) {
    if (s.empty()) return false;
    for (char c : s)
        if (c < '0' || c > '9') return false;
    return true;
}

void take_numeric_opt(const char* name, std::string attached, int& i, int argc, char** argv,
                      int min_v, int max_v, int& dest, bool optional) {
    std::string val = attached;
    if (val.empty() && i + 1 < argc && argv[i + 1][0] != '-') {
        val = argv[++i];
    } else if (val.empty() && i + 1 < argc && is_digits(argv[i + 1])) {
        val = argv[++i];
    }
    if (val.empty()) {
        if (!optional) {
            std::cerr << "Error: " << name << " requires a numeric argument\n";
            std::exit(1);
        }
        return;
    }
    int n = std::atoi(val.c_str());
    if (n < min_v || n > max_v) {
        std::cerr << "Error: unsupported " << name << " value: " << val << "\n";
        std::exit(1);
    }
    dest = n;
}

std::string take_string(const char* name, std::string attached, int& i, int argc, char** argv) {
    if (!attached.empty()) return attached;
    if (i + 1 < argc) return argv[++i];
    std::cerr << "Error: " << name << " requires an argument\n";
    std::exit(1);
}

}  // namespace

void apply_basic(Options& o) {
    disable_short(o);
    o.show_battery = true;
    o.show_cpu_basic = true;
    o.show_raid_basic = true;
    o.show_disk_total = true;
    o.show_graphic = true;
    o.show_info = true;
    o.show_machine = true;
    o.show_network = true;
    o.show_system = true;
}

void apply_expanded(Options& o) {
    disable_short(o);
    o.show_audio = true;
    o.show_battery = true;
    o.show_bluetooth = true;
    o.show_cpu = true;
    o.show_disk = true;
    o.show_graphic = true;
    o.show_graphic_full = true;
    o.show_info = true;
    o.show_machine = true;
    o.show_network = true;
    o.show_network_advanced = true;
    o.show_partition = true;
    o.show_raid = true;
    o.show_sensor = true;
    o.show_swap = true;
    o.show_system = true;
}

void apply_verbosity(Options& o, int level) {
    disable_short(o);
    if (level == 0) {
        o.show_short = true;
        return;
    }
    if (level >= 1) {
        o.show_cpu_basic = true;
        o.show_disk_total = true;
        o.show_graphic = true;
        o.show_info = true;
        o.show_system = true;
    }
    if (level >= 2) {
        o.show_battery = true;
        o.show_disk_basic = true;
        o.show_raid_basic = true;
        o.show_machine = true;
        o.show_network = true;
    }
    if (level >= 3) {
        o.show_network_advanced = true;
        o.show_cpu = true;
        o.show_cpu_basic = false;
        o.extra = std::max(o.extra, 1);
    }
    if (level >= 4) {
        o.show_disk = true;
        o.show_partition = true;
    }
    if (level >= 5) {
        o.show_audio = true;
        o.show_bluetooth = true;
        o.show_graphic_full = true;
        o.show_label = true;
        o.show_optical = true;
        o.show_raid = true;
        o.show_ram = true;
        o.show_sensor = true;
        o.show_swap = true;
        o.show_uuid = true;
    }
    if (level >= 6) {
        o.show_optical = true;
        o.show_partition_full = true;
        o.show_unmounted = true;
        o.show_usb = true;
        o.extra = std::max(o.extra, 2);
    }
    if (level >= 7) {
        o.show_battery_forced = true;
        o.show_bluetooth_forced = true;
        o.show_cpu_flag = true;
        o.show_ip = true;
        o.show_logical = true;
        o.extra = std::max(o.extra, 3);
    }
    if (level >= 8) {
        o.admin = true;
        o.show_process = true;
        o.show_ps_cpu = true;
        o.show_ps_mem = true;
        o.show_repo = true;
        o.show_slot = true;
    }
}

static void handle_long(Options& o, const std::string& name, std::string val, int& i, int argc,
                        char** argv) {
    auto eq = name.find('=');
    std::string key = name;
    if (eq != std::string::npos) {
        key = name.substr(0, eq);
        val = name.substr(eq + 1);
    }

    auto set_true = [&](auto member) {
        disable_short(o);
        member = true;
    };

    if (key == "admin") o.admin = true;
    else if (key == "audio") { disable_short(o); o.show_audio = true; }
    else if (key == "basic") apply_basic(o);
    else if (key == "battery") { disable_short(o); o.show_battery = true; o.show_battery_forced = true; }
    else if (key == "color") {
        int n = 0;
        take_numeric_opt("--color", val, i, argc, argv, 0, 99, n, false);
        o.color = n;
    }
    else if (key == "cpu") { disable_short(o); o.show_cpu = true; }
    else if (key == "config" || key == "configuration" || key == "configs") o.show_configs = true;
    else if (key == "disk-full" || key == "optical") { disable_short(o); o.show_disk = true; o.show_optical = true; }
    else if (key == "disk") { disable_short(o); o.show_disk = true; }
    else if (key == "bluetooth") { disable_short(o); o.show_bluetooth = true; o.show_bluetooth_forced = true; }
    else if (key == "edid") { o.admin = true; disable_short(o); o.show_graphic = true; o.show_graphic_full = true; }
    else if (key == "flags" || key == "flag") { disable_short(o); o.show_cpu = true; o.show_cpu_flag = true; }
    else if (key == "full" || key == "expanded") apply_expanded(o);
    else if (key == "graphics" || key == "graphic") {
        disable_short(o); o.show_graphic = true; o.show_graphic_full = true;
    }
    else if (key == "help") o.show_help = true;
    else if (key == "ip") {
        disable_short(o); o.show_ip = true; o.show_network = true; o.show_network_advanced = true;
    }
    else if (key == "info") { disable_short(o); o.show_info = true; }
    else if (key == "swap" || key == "swaps") { disable_short(o); o.show_swap = true; }
    else if (key == "usb") { disable_short(o); o.show_usb = true; }
    else if (key == "labels" || key == "label") o.show_label = true;
    else if (key == "logical" || key == "lvm") { disable_short(o); o.show_logical = true; }
    else if (key == "memory") { disable_short(o); o.show_ram = true; }
    else if (key == "memory-modules" || key == "mm") {
        disable_short(o); o.show_ram = true; o.show_ram_modules = true;
    }
    else if (key == "memory-short" || key == "ms") {
        disable_short(o); o.show_ram = true; o.show_ram_short = true;
    }
    else if (key == "machine") { disable_short(o); o.show_machine = true; }
    else if (key == "network-advanced") {
        disable_short(o); o.show_network = true; o.show_network_advanced = true;
    }
    else if (key == "network") { disable_short(o); o.show_network = true; }
    else if (key == "unmounted") { disable_short(o); o.show_unmounted = true; }
    else if (key == "partition-full" || key == "partitions-full") {
        disable_short(o); o.show_partition_full = true;
    }
    else if (key == "partitions" || key == "partition") { disable_short(o); o.show_partition = true; }
    else if (key == "repos" || key == "repo") { disable_short(o); o.show_repo = true; }
    else if (key == "raid") { disable_short(o); o.show_raid = true; }
    else if (key == "sensors" || key == "sensor") { disable_short(o); o.show_sensor = true; }
    else if (key == "slots" || key == "slot") { disable_short(o); o.show_slot = true; }
    else if (key == "system") { disable_short(o); o.show_system = true; }
    else if (key == "processes" || key == "process") {
        disable_short(o);
        o.show_process = true;
        auto arg = val.empty() ? (i + 1 < argc && argv[i + 1][0] != '-' ? std::string(argv[++i]) : std::string("cm"))
                               : val;
        if (arg.find('c') != std::string::npos) o.show_ps_cpu = true;
        if (arg.find('m') != std::string::npos) o.show_ps_mem = true;
        std::string num;
        for (char c : arg)
            if (c >= '0' && c <= '9') num.push_back(c);
        if (!num.empty()) o.ps_count = std::atoi(num.c_str());
        if (!o.show_ps_cpu && !o.show_ps_mem) {
            o.show_ps_cpu = true;
            o.show_ps_mem = true;
        }
    }
    else if (key == "uuid") o.show_uuid = true;
    else if (key == "verbosity") {
        int n = 0;
        take_numeric_opt("--verbosity", val, i, argc, argv, 0, 8, n, false);
        apply_verbosity(o, n);
    }
    else if (key == "version") o.show_version = true;
    else if (key == "version-short" || key == "vs") o.show_version_short = true;
    else if (key == "weather") { disable_short(o); o.show_weather = true; }
    else if (key == "weather-location") {
        disable_short(o);
        o.show_weather = true;
        o.weather_location = take_string("--weather-location", val, i, argc, argv);
    }
    else if (key == "weather-unit") {
        o.weather_unit = take_string("--weather-unit", val, i, argc, argv);
    }
    else if (key == "extra") {
        int n = -1;
        if (!val.empty() || (i + 1 < argc && is_digits(argv[i + 1]))) {
            take_numeric_opt("--extra", val, i, argc, argv, 1, 3, n, true);
            o.extra = n;
        } else {
            o.extra++;
        }
    }
    else if (key == "width") {
        int n = 80;
        take_numeric_opt("--width", val, i, argc, argv, 1, 10000, n, true);
        o.width = n;
    }
    else if (key == "filter") o.filter = true;
    else if (key == "filter-all" || key == "za") o.filter = true;
    else if (key == "filter-override" || key == "no-filter") o.filter_override = true;
    else if (key == "output" || key == "export") {
        o.output = take_string("--output", val, i, argc, argv);
    }
    else if (key == "recommends") o.show_recommends = true;
    else if (key == "separator" || key == "sep") {
        o.separator = take_string("--separator", val, i, argc, argv);
    }
    else {
        std::cerr << "Error: unknown option --" << key << "\n";
        std::exit(1);
    }
    (void)set_true;
}

static void handle_cluster(Options& o, const std::string& cluster, int& i, int argc, char** argv) {
    for (size_t p = 0; p < cluster.size(); ++p) {
        char c = cluster[p];
        std::string rest = cluster.substr(p + 1);
        auto consume_rest_num = [&](int& dest, int min_v, int max_v, bool optional) {
            if (!rest.empty() && (rest[0] == '-' || (rest[0] >= '0' && rest[0] <= '9'))) {
                dest = std::atoi(rest.c_str());
                if (dest < min_v || dest > max_v) {
                    std::cerr << "Error: bad numeric value for -" << c << "\n";
                    std::exit(1);
                }
                p = cluster.size();
                return;
            }
            if (i + 1 < argc && (is_digits(argv[i + 1]) || argv[i + 1][0] != '-')) {
                dest = std::atoi(argv[++i]);
                p = cluster.size();
                return;
            }
            if (!optional) {
                std::cerr << "Error: -" << c << " needs a number\n";
                std::exit(1);
            }
        };

        switch (c) {
            case 'a': o.admin = true; break;
            case 'A': disable_short(o); o.show_audio = true; break;
            case 'b': apply_basic(o); break;
            case 'B': disable_short(o); o.show_battery = true; o.show_battery_forced = true; break;
            case 'c': {
                int n = 0;
                consume_rest_num(n, 0, 99, false);
                o.color = n;
                break;
            }
            case 'C': disable_short(o); o.show_cpu = true; break;
            case 'd': disable_short(o); o.show_disk = true; o.show_optical = true; break;
            case 'D': disable_short(o); o.show_disk = true; break;
            case 'e': apply_expanded(o); break;
            case 'E': disable_short(o); o.show_bluetooth = true; o.show_bluetooth_forced = true; break;
            case 'f': disable_short(o); o.show_cpu = true; o.show_cpu_flag = true; break;
            case 'F': apply_expanded(o); break;
            case 'G': disable_short(o); o.show_graphic = true; o.show_graphic_full = true; break;
            case 'h': o.show_help = true; break;
            case 'i':
                disable_short(o);
                o.show_ip = true;
                o.show_network = true;
                o.show_network_advanced = true;
                break;
            case 'I': disable_short(o); o.show_info = true; break;
            case 'j': disable_short(o); o.show_swap = true; break;
            case 'J': disable_short(o); o.show_usb = true; break;
            case 'l': o.show_label = true; break;
            case 'L': disable_short(o); o.show_logical = true; break;
            case 'm': disable_short(o); o.show_ram = true; break;
            case 'M': disable_short(o); o.show_machine = true; break;
            case 'n': disable_short(o); o.show_network = true; o.show_network_advanced = true; break;
            case 'N': disable_short(o); o.show_network = true; break;
            case 'o': disable_short(o); o.show_unmounted = true; break;
            case 'p': disable_short(o); o.show_partition_full = true; break;
            case 'P': disable_short(o); o.show_partition = true; break;
            case 'r': disable_short(o); o.show_repo = true; break;
            case 'R': disable_short(o); o.show_raid = true; break;
            case 's': disable_short(o); o.show_sensor = true; break;
            case 'S': disable_short(o); o.show_system = true; break;
            case 't': {
                disable_short(o);
                o.show_process = true;
                std::string arg = rest;
                if (arg.empty() && i + 1 < argc && argv[i + 1][0] != '-') arg = argv[++i];
                if (arg.empty()) arg = "cm";
                if (arg.find('c') != std::string::npos) o.show_ps_cpu = true;
                if (arg.find('m') != std::string::npos) o.show_ps_mem = true;
                std::string num;
                for (char ch : arg)
                    if (ch >= '0' && ch <= '9') num.push_back(ch);
                if (!num.empty()) o.ps_count = std::atoi(num.c_str());
                if (!o.show_ps_cpu && !o.show_ps_mem) {
                    o.show_ps_cpu = o.show_ps_mem = true;
                }
                p = cluster.size();
                break;
            }
            case 'u': o.show_uuid = true; break;
            case 'v': {
                int n = 0;
                consume_rest_num(n, 0, 8, false);
                apply_verbosity(o, n);
                break;
            }
            case 'V': o.show_version = true; break;
            case 'w': disable_short(o); o.show_weather = true; break;
            case 'W':
                disable_short(o);
                o.show_weather = true;
                o.weather_location = take_string("-W", rest, i, argc, argv);
                p = cluster.size();
                break;
            case 'x': {
                int extra_add = 1;
                while (p + extra_add < cluster.size() && cluster[p + extra_add] == 'x') ++extra_add;
                o.extra += extra_add;
                p += extra_add - 1;
                break;
            }
            case 'y': {
                int n = 80;
                consume_rest_num(n, 1, 10000, true);
                o.width = n;
                break;
            }
            case 'z': o.filter = true; break;
            case 'Z': o.filter_override = true; break;
            default:
                std::cerr << "Error: unknown option -" << c << "\n";
                std::exit(1);
        }
    }
}

Options parse_args(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--") break;
        if (starts_with(a, "--")) {
            handle_long(o, a.substr(2), {}, i, argc, argv);
        } else if (a.size() > 1 && a[0] == '-') {
            handle_cluster(o, a.substr(1), i, argc, argv);
        } else {
            std::cerr << "Error: unexpected argument " << a << "\n";
            std::exit(1);
        }
    }
    if (o.filter_override) o.filter = false;
    return o;
}

void print_version(bool short_form) {
    if (short_form) {
        std::cout << INXI_CPP_VERSION << '\n';
        return;
    }
    std::cout << "inxi-cpp " << INXI_CPP_VERSION
              << " (C++ port of inxi " << INXI_BASED_ON << ")\n"
              << "License: GNU GPL v3+\n"
              << "Upstream Perl inxi: https://codeberg.org/smxi/inxi\n";
}

void print_help() {
    std::cout << R"(inxi-cpp - C++ port of inxi, a CLI system information tool

Usage:
  inxi
  inxi [-AbBCdDeEfFGhiIjJlLmMnNopPrRsSuVwyYzZ]
  inxi [-c NUMBER] [-v NUMBER] [-t [c|m|cm][NUMBER]] [-y WIDTH]

Standard options (short forms group, e.g. inxi -Fxxz):
  -A, --audio             Audio devices and sound servers
  -b, --basic             Basic report (System, Machine, Battery, CPU, Graphics,
                          Network, Disk totals, Info)
  -B, --battery           Battery
  -C, --cpu               Full CPU report
  -d, --optical           Disks plus optical drives
  -D, --disk              Hard/SSD drives
  -e, -F, --expanded      Expanded report (most hardware + sensors + partitions)
  -E, --bluetooth         Bluetooth
  -f, --flags             CPU flags
  -G, --graphics          Graphics devices, display, APIs
  -i, --ip                Network plus IP addresses
  -I, --info              Processes, uptime, memory, compilers, packages
  -j, --swap              Swap
  -J, --usb               USB devices
  -L, --logical           Logical volumes (device-mapper)
  -m, --memory            RAM / memory report
  -M, --machine           Machine / board / firmware
  -n, --network-advanced  Network plus interface state
  -N, --network           Network devices
  -o, --unmounted         Unmounted partitions
  -p, --partition-full    All partitions
  -P, --partitions        Main partitions
  -r, --repos             Package repositories
  -R, --raid              RAID
  -s, --sensors           Sensors (hwmon)
  -S, --system            Kernel, distro, desktop
  -t, --processes         Top CPU/memory processes
  -w, --weather           Weather (requires network tool)
  -x, -xx, -xxx, -a       Extra / admin detail
  -z, --filter            Filter user paths
  -c, --color NUMBER      0 disables color
  -y, --width NUMBER      Wrap width
  -v, --verbosity 0-8     Preset report levels
  --output json|xml       Machine-readable output
  --recommends            Show recommended helper tools
  -V, --version           Version
  -h, --help              This help

This is a native C++ rewrite aimed at Linux. BSD/Windows paths from Perl inxi
are not fully ported. Output matches inxi's key: value layout.
)";
}

}  // namespace inxi
