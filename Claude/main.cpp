// inxi-cpp -- a partial C++ port of inxi (https://github.com/smxi/inxi)
//
// inxi itself is a single ~36,700-line Perl script covering dozens of
// command-line flags built up over roughly 20 years. This port re-implements
// its most commonly used report sections in modular C++, reading the same
// underlying /proc, /sys, and standard command-line tools inxi does.
// See README.md for exactly what is and isn't covered.

#include "common.h"
#include "system_module.h"
#include "cpu_module.h"
#include "memory_module.h"
#include "graphics_module.h"
#include "network_module.h"
#include "storage_module.h"
#include "info_module.h"
#include "summary.h"
#include "version.h"

#include <iostream>
#include <set>

using namespace mod;
using namespace util;

namespace {

void printHelp() {
    std::cout <<
"inxi-cpp " << PROGRAM_VERSION << " -- a partial C++ port of inxi (https://github.com/smxi/inxi)\n\n"
"Usage: inxi-cpp [options]\n\n"
"With no options, prints a short one-line summary.\n\n"
"Section options (combine freely, e.g. -SCM):\n"
"  -S            System: host, kernel, arch, desktop, distro\n"
"  -M            Machine: DMI system/board/BIOS info\n"
"  -C            CPU: model, cores, speed, cache\n"
"  -m            Memory: RAM total/used/available (not auto-included by -F)\n"
"  -G            Graphics: GPU device(s), display server\n"
"  -N            Network: interfaces, driver, state, MAC\n"
"  -D            Drives: physical block devices\n"
"  -P            Partition: mounted filesystems, usage\n"
"  -j            Swap: swap devices/files in use\n"
"  -I            Info: uptime, processes, packages, shell\n"
"  -F            Full: all of the above except -m, in order\n\n"
"Modifiers:\n"
"  -f            Show all CPU flags in full. Triggers -C.\n"
"  -z            Mask the last 3 octets of MAC addresses (use with -N/-F)\n"
"  -c 0|1        Force color off/on (default: auto-detect terminal)\n"
"  -h, --help    Show this help\n"
"  -v, --version Show version\n\n"
"Not yet ported from the original inxi: Audio (-A), Bluetooth (-E),\n"
"Battery (-B), RAID (-R), Sensors (-s), Weather (-w), USB (-J), repos\n"
"(-r), logical/LVM (-L), verbosity levels (-v N), and JSON/-x extra-data\n"
"output. See README.md for the full list and how to add a new section.\n";
}

} // namespace

int main(int argc, char** argv) {
    std::set<char> sections;
    bool showAllFlags = false;
    bool filterMac = false;
    bool colorOn = stdoutIsTty();

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help") { printHelp(); return 0; }
        if (arg == "--version") { std::cout << PROGRAM_NAME << " " << PROGRAM_VERSION << "\n"; return 0; }
        if (arg == "--flags") { showAllFlags = true; continue; }
        if (arg == "--filter") { filterMac = true; continue; }
        if (arg == "--color") {
            if (i + 1 < argc) colorOn = (std::string(argv[++i]) != "0");
            continue;
        }

        if (arg.size() >= 2 && arg[0] == '-' && arg[1] != '-') {
            for (size_t j = 1; j < arg.size(); ++j) {
                char c = arg[j];
                switch (c) {
                    case 'h': printHelp(); return 0;
                    case 'v': std::cout << PROGRAM_NAME << " " << PROGRAM_VERSION << "\n"; return 0;
                    case 'f': showAllFlags = true; break;
                    case 'z': filterMac = true; break;
                    case 'c': {
                        // Accepts "-c0"/"-c1" or "-c 0"/"-c 1".
                        std::string rest = arg.substr(j + 1);
                        if (!rest.empty()) {
                            colorOn = (rest != "0");
                            j = arg.size();
                        } else if (i + 1 < argc) {
                            colorOn = (std::string(argv[++i]) != "0");
                        }
                        break;
                    }
                    case 'S': case 'M': case 'C': case 'm': case 'G':
                    case 'N': case 'D': case 'P': case 'j': case 'I':
                    case 'F':
                        sections.insert(c);
                        break;
                    default:
                        std::cerr << "inxi-cpp: unknown option -" << c << " (see --help)\n";
                        break;
                }
            }
            continue;
        }

        std::cerr << "inxi-cpp: unrecognized argument '" << arg << "' (see --help)\n";
    }

    // Matches inxi: "-f, --flags ... Triggers -C."
    if (showAllFlags) sections.insert('C');

    if (sections.empty()) {
        printDefaultSummary(std::cout, colorOn);
        return 0;
    }

    bool full = sections.count('F') > 0;

    if (full || sections.count('S')) printSystemSection(std::cout, colorOn);
    if (full || sections.count('M')) printMachineSection(std::cout, colorOn);
    if (sections.count('m'))         printMemorySection(std::cout, colorOn); // -F never auto-includes -m
    if (full || sections.count('C')) printCpuSection(std::cout, colorOn, showAllFlags);
    if (full || sections.count('G')) printGraphicsSection(std::cout, colorOn);
    if (full || sections.count('N')) printNetworkSection(std::cout, colorOn, filterMac);
    if (full || sections.count('D')) printDrivesSection(std::cout, colorOn);
    if (full || sections.count('P')) printPartitionsSection(std::cout, colorOn);
    if (full || sections.count('j')) printSwapSection(std::cout, colorOn);
    if (full || sections.count('I')) printInfoSection(std::cout, colorOn);

    return 0;
}
