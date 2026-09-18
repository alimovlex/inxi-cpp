#include "inxi/report.hpp"

#include "inxi/collect.hpp"
#include "inxi/util.hpp"

#include <iostream>

namespace inxi {

Report build_report(const Options& opt) {
    Report r;
    if (opt.show_short) {
        collect_short(r, opt);
        return r;
    }
    if (opt.show_system) collect_system(r, opt);
    if (opt.show_machine) collect_machine(r, opt);
    if (opt.show_battery || opt.show_battery_forced) collect_battery(r, opt);
    if (opt.show_ram) collect_ram(r, opt);
    if (opt.show_slot) collect_slots(r, opt);
    if (opt.show_cpu) collect_cpu(r, opt, false);
    else if (opt.show_cpu_basic) collect_cpu(r, opt, true);
    if (opt.show_graphic) collect_graphics(r, opt);
    if (opt.show_audio) collect_audio(r, opt);
    if (opt.show_network) collect_network(r, opt);
    if (opt.show_bluetooth || opt.show_bluetooth_forced) collect_bluetooth(r, opt);
    if (opt.show_logical) collect_logical(r, opt);
    if (opt.show_raid || opt.show_raid_basic) collect_raid(r, opt);
    if (opt.show_disk || opt.show_disk_basic || opt.show_disk_total || opt.show_optical)
        collect_drives(r, opt);
    if (opt.show_partition || opt.show_partition_full) collect_partitions(r, opt);
    if (opt.show_swap) collect_swap(r, opt);
    if (opt.show_unmounted) collect_unmounted(r, opt);
    if (opt.show_usb) collect_usb(r, opt);
    if (opt.show_sensor) collect_sensors(r, opt);
    if (opt.show_repo) collect_repos(r, opt);
    if (opt.show_process) collect_processes(r, opt);
    if (opt.show_weather) collect_weather(r, opt);
    if (opt.show_info) collect_info(r, opt);
    return r;
}

void print_recommends() {
    struct Item {
        const char* name;
        const char* why;
    };
    Item items[] = {
        {"lspci", "PCI device names (also uses pci.ids)"},
        {"lsusb", "USB listing fallback"},
        {"dmidecode", "Memory modules, PCI slots, extra DMI"},
        {"glxinfo", "OpenGL API line"},
        {"eglinfo", "EGL info"},
        {"vulkaninfo", "Vulkan info"},
        {"xrandr", "Display resolutions (X11)"},
        {"xdpyinfo", "X.Org version"},
        {"ip", "Interface IP addresses (-i)"},
        {"curl", "Weather (-w) and WAN IP"},
        {"sensors", "lm-sensors (hwmon sysfs is used first)"},
        {"smartctl", "Drive SMART (not yet ported)"},
        {"bluetoothctl", "Bluetooth details"},
    };
    std::cout << "Recommended helper programs for inxi-cpp:\n";
    for (auto& it : items) {
        auto p = which(it.name);
        std::cout << "  " << (p.empty() ? "MISSING" : "present") << "  " << it.name << " - " << it.why
                  << "\n";
    }
}

}  // namespace inxi
