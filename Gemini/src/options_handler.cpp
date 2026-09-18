// options_handler.cpp
#include "options_handler.h"
#include "colors.h"
#include "utils/string_utils.h"
#include <cstdlib>
#include <iostream>

namespace inxi {

OptionsHandler::OptionsHandler(GlobalState& state) : state_(state) {}

void OptionsHandler::set_extra(int level) {
    state_.extra = level;
}

void OptionsHandler::apply_color(int scheme) {
    state_.colors.scheme = scheme;
    set_color_scheme(state_, scheme);
}

void OptionsHandler::show_version() {
    std::cout << state_.self_name << " " << state_.self_version
              << "-" << state_.self_patch << " (" << state_.self_date << ")\n";
    std::exit(0);
}

void OptionsHandler::show_options() {
    std::cout << "inxi " << state_.self_version << "\n\n"
              << "Usage: inxi [options]\n\n"
              << "Output Control Options:\n"
              << "  -A, --audio          Show Audio/sound card information.\n"
              << "  -b, --basic          Show basic system output (equivalent to: -CdrvG).\n"
              << "  -B, --battery        Show battery status.\n"
              << "  -c, --color [0-32]   Set color scheme number (0 turns colors off).\n"
              << "  -C, --cpu            Show full CPU output.\n"
              << "  -d, --disk-full      Show optical and all drives.\n"
              << "  -D, --disk           Show Hard Disk info.\n"
              << "  -E, --bluetooth      Show Bluetooth information.\n"
              << "  -F, --full           Show full inxi output.\n"
              << "  -G, --graphics       Show Graphic information.\n"
              << "  -h, --help           Show this help menu.\n"
              << "  -I, --info           Show general system Information.\n"
              << "  -i, --ip             Show WAN and LAN IP addresses.\n"
              << "  -j, --swap           Show Swap partition/file info.\n"
              << "  -J, --usb            Show USB devices.\n"
              << "  -m, --memory         Show Memory (RAM) information.\n"
              << "  -M, --machine        Show Machine data (Motherboard, BIOS, etc).\n"
              << "  -N, --network        Show Network card information.\n"
              << "  -o, --unmounted      Show unmounted partitions.\n"
              << "  -p, --partitions     Show mounted partition information.\n"
              << "  -P, --partitions-full Show full partition info.\n"
              << "  -r, --repos          Show package repository information.\n"
              << "  -R, --raid           Show RAID information.\n"
              << "  -s, --sensors        Show Sensors (temperatures, fans, voltages).\n"
              << "  -S, --system         Show System information (host, kernel, distro).\n"
              << "  -v, --verbosity [0-8] Set verbosity level.\n"
              << "  -V, --version        Show inxi version information.\n"
              << "  -w, --weather        Show local weather report.\n"
              << "  -x                   Add extra system data.\n"
              << "  -xx                  Add further extra data.\n"
              << "  -xxx                 Add all available extra data.\n";
    std::exit(0);
}

void OptionsHandler::parse(int argc, char* argv[]) {
    if (argc <= 1) {
        // Default mode: no arguments given
        return;
    }

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            show_options();
        } else if (arg == "-V" || arg == "--version") {
            show_version();
        } else if (arg == "-b" || arg == "--basic") {
            state_.show.system = true;
            state_.show.machine = true;
            state_.show.battery = true;
            state_.show.cpu = true;
            state_.show.graphics = true;
            state_.show.network = true;
            state_.show.disk = true;
            state_.show.info = true;
        } else if (arg == "-F" || arg == "--full") {
            state_.show.system = true;
            state_.show.machine = true;
            state_.show.battery = true;
            state_.show.cpu = true;
            state_.show.graphics = true;
            state_.show.audio = true;
            state_.show.network = true;
            state_.show.bluetooth = true;
            state_.show.disk = true;
            state_.show.partition = true;
            state_.show.swap = true;
            state_.show.sensor = true;
            state_.show.info = true;
        } else if (arg == "-A" || arg == "--audio") {
            state_.show.audio = true;
        } else if (arg == "-B" || arg == "--battery") {
            state_.show.battery = true;
        } else if (arg == "-C" || arg == "--cpu") {
            state_.show.cpu = true;
        } else if (arg == "-D" || arg == "--disk") {
            state_.show.disk = true;
        } else if (arg == "-d" || arg == "--disk-full") {
            state_.show.disk = true;
            state_.show.disk_total = true;
        } else if (arg == "-E" || arg == "--bluetooth") {
            state_.show.bluetooth = true;
        } else if (arg == "-G" || arg == "--graphics") {
            state_.show.graphics = true;
        } else if (arg == "-I" || arg == "--info") {
            state_.show.info = true;
            state_.show.process = true;
        } else if (arg == "-i" || arg == "--ip") {
            state_.show.ip = true;
            state_.show.network = true;
        } else if (arg == "-j" || arg == "--swap") {
            state_.show.swap = true;
        } else if (arg == "-J" || arg == "--usb") {
            state_.show.usb = true;
        } else if (arg == "-M" || arg == "--machine") {
            state_.show.machine = true;
        } else if (arg == "-m" || arg == "--memory") {
            state_.show.memory = true;
        } else if (arg == "-N" || arg == "--network") {
            state_.show.network = true;
        } else if (arg == "-o" || arg == "--unmounted") {
            state_.show.unmounted = true;
        } else if (arg == "-p" || arg == "--partitions" || arg == "-P" || arg == "--partitions-full") {
            state_.show.partition = true;
        } else if (arg == "-r" || arg == "--repos") {
            state_.show.repo = true;
        } else if (arg == "-R" || arg == "--raid") {
            state_.show.raid = true;
        } else if (arg == "-s" || arg == "--sensors") {
            state_.show.sensor = true;
        } else if (arg == "-S" || arg == "--system") {
            state_.show.system = true;
        } else if (arg == "-w" || arg == "-W" || arg == "--weather") {
            state_.show.weather = true;
        } else if (arg == "-c" || arg == "--color") {
            if (i + 1 < argc && utils::is_int(argv[i + 1])) {
                apply_color(std::stoi(argv[++i]));
            }
        } else if (arg.rfind("-c", 0) == 0 && utils::is_int(arg.substr(2))) {
            apply_color(std::stoi(arg.substr(2)));
        } else if (arg == "-x") {
            set_extra(1);
        } else if (arg == "-xx") {
            set_extra(2);
        } else if (arg == "-xxx") {
            set_extra(3);
        } else if (arg == "-z" || arg == "--filter") {
            state_.filter = true;
        } else if (arg == "-a" || arg == "--admin") {
            state_.b_admin = true;
            if (state_.extra == 0) set_extra(1);
        } else if (arg.rfind("-v", 0) == 0) {
            int v = 1;
            if (arg.size() > 2 && utils::is_int(arg.substr(2))) {
                v = std::stoi(arg.substr(2));
            } else if (i + 1 < argc && utils::is_int(argv[i + 1])) {
                v = std::stoi(argv[++i]);
            }
            if (v == 0) {
                // Short single line
            } else if (v >= 1) {
                state_.show.system = true;
                state_.show.cpu = true;
                state_.show.graphics = true;
                state_.show.disk = true;
                state_.show.info = true;
            }
            if (v >= 2) {
                state_.show.machine = true;
                state_.show.battery = true;
                state_.show.network = true;
            }
            if (v >= 3) {
                state_.show.audio = true;
                state_.show.sensor = true;
                state_.show.partition = true;
            }
            if (v >= 4) {
                state_.show.usb = true;
                state_.show.bluetooth = true;
            }
        } else if (arg[0] == '-' && arg.size() > 1 && arg[1] != '-') {
            // Clustered short options, e.g. -SCM, -b, etc.
            for (size_t c = 1; c < arg.size(); ++c) {
                char ch = arg[c];
                if (ch == 'b') {
                    state_.show.system = true;
                    state_.show.machine = true;
                    state_.show.battery = true;
                    state_.show.cpu = true;
                    state_.show.graphics = true;
                    state_.show.network = true;
                    state_.show.disk = true;
                    state_.show.info = true;
                } else if (ch == 'F') {
                    state_.show.system = true;
                    state_.show.machine = true;
                    state_.show.battery = true;
                    state_.show.cpu = true;
                    state_.show.graphics = true;
                    state_.show.audio = true;
                    state_.show.network = true;
                    state_.show.bluetooth = true;
                    state_.show.disk = true;
                    state_.show.partition = true;
                    state_.show.swap = true;
                    state_.show.sensor = true;
                    state_.show.info = true;
                } else if (ch == 'A') state_.show.audio = true;
                else if (ch == 'B') state_.show.battery = true;
                else if (ch == 'C') state_.show.cpu = true;
                else if (ch == 'D') state_.show.disk = true;
                else if (ch == 'E') state_.show.bluetooth = true;
                else if (ch == 'G') state_.show.graphics = true;
                else if (ch == 'I') { state_.show.info = true; state_.show.process = true; }
                else if (ch == 'i') { state_.show.ip = true; state_.show.network = true; }
                else if (ch == 'j') state_.show.swap = true;
                else if (ch == 'J') state_.show.usb = true;
                else if (ch == 'M') state_.show.machine = true;
                else if (ch == 'm') state_.show.memory = true;
                else if (ch == 'N') state_.show.network = true;
                else if (ch == 'o') state_.show.unmounted = true;
                else if (ch == 'p' || ch == 'P') state_.show.partition = true;
                else if (ch == 'r') state_.show.repo = true;
                else if (ch == 'R') state_.show.raid = true;
                else if (ch == 's') state_.show.sensor = true;
                else if (ch == 'S') state_.show.system = true;
                else if (ch == 'x') set_extra(std::min(3, state_.extra + 1));
                else if (ch == 'z') state_.filter = true;
                else if (ch == 'a') {
                    state_.b_admin = true;
                    if (state_.extra == 0) set_extra(1);
                }
            }
        }
    }

    post_process();
}

void OptionsHandler::post_process() {
    validate();
}

void OptionsHandler::process_updater() {}

void OptionsHandler::validate() {}

}  // namespace inxi
