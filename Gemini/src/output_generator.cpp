// output_generator.cpp
#include "output_generator.h"
#include "output.h"
#include "items/audio_item.h"
#include "items/battery_item.h"
#include "items/bluetooth_item.h"
#include "items/cpu_item.h"
#include "items/disk_item.h"
#include "items/graphics_item.h"
#include "items/info_item.h"
#include "items/machine_item.h"
#include "items/memory_item.h"
#include "items/network_item.h"
#include "items/partition_item.h"
#include "items/raid_item.h"
#include "items/remaining_items.h"
#include "items/sensor_item.h"
#include "items/system_item.h"

namespace inxi {

OutputGenerator::OutputGenerator(GlobalState& state) : state_(state) {}

void OutputGenerator::generate() {
    Output output(state_);
    std::vector<OutputRow> all_rows;

    bool any_show = state_.show.system || state_.show.machine || state_.show.battery ||
                    state_.show.cpu || state_.show.graphics || state_.show.audio ||
                    state_.show.network || state_.show.bluetooth || state_.show.raid ||
                    state_.show.disk || state_.show.partition || state_.show.unmounted ||
                    state_.show.usb || state_.show.sensor || state_.show.repo ||
                    state_.show.process || state_.show.swap || state_.show.weather ||
                    state_.show.info || state_.show.memory;

    if (!any_show) {
        // Default summary: 3 lines
        // 1. CPU
        CpuItem cpu(state_);
        auto cpu_rows = cpu.get();
        if (!cpu_rows.empty()) {
            all_rows.push_back(cpu_rows[0]);
        }

        // 2. Kernel, Up, Mem
        SystemItem sys(state_);
        auto sys_rows = sys.get();
        MemoryItem mem(state_);
        auto mem_rows = mem.get();
        GeneralInfoItem info(state_);
        auto info_rows = info.get();

        std::vector<Field> f2;
        if (!sys_rows.empty() && !sys_rows[0].fields.empty()) {
            for (const auto& f : sys_rows[0].fields) {
                if (f.key == "Kernel") f2.push_back(f);
                else if (f.key == "arch") f2.push_back(Field{"", f.value});
            }
        }
        if (!info_rows.empty() && !info_rows[0].fields.empty()) {
            for (const auto& f : info_rows[0].fields) {
                if (f.key == "Uptime") f2.push_back(Field{"Up", f.value});
            }
        }
        if (!mem_rows.empty() && !mem_rows[0].fields.empty()) {
            for (const auto& f : mem_rows[0].fields) {
                if (f.key == "RAM") {
                    f2.push_back(Field{"Mem", f.value.substr(f.value.find("used: ") + 6)});
                }
            }
        }
        all_rows.push_back(OutputRow{"", f2, false});

        // 3. Storage, Procs, Shell, inxi
        DiskItem disk(state_);
        auto disk_rows = disk.get();
        std::vector<Field> f3;
        if (!disk_rows.empty() && !disk_rows[0].fields.empty()) {
            for (const auto& f : disk_rows[0].fields) {
                if (f.key == "Local Storage") {
                    f3.push_back(Field{"Storage", f.value});
                }
            }
        }
        if (!info_rows.empty() && !info_rows[0].fields.empty()) {
            for (const auto& f : info_rows[0].fields) {
                if (f.key == "Processes") f3.push_back(Field{"Procs", f.value});
                else if (f.key == "Shell") f3.push_back(f);
                else if (f.key == state_.self_name) f3.push_back(f);
            }
        }
        all_rows.push_back(OutputRow{"", f3, false});

        output.handle(all_rows);
        return;
    }

    if (state_.show.system) {
        SystemItem item(state_);
        auto r = item.get();
        all_rows.insert(all_rows.end(), r.begin(), r.end());
    }
    if (state_.show.machine) {
        MachineItem item(state_);
        auto r = item.get();
        all_rows.insert(all_rows.end(), r.begin(), r.end());
    }
    if (state_.show.battery) {
        BatteryItem item(state_);
        auto r = item.get();
        all_rows.insert(all_rows.end(), r.begin(), r.end());
    }
    if (state_.show.memory) {
        MemoryItem item(state_);
        auto r = item.get();
        all_rows.insert(all_rows.end(), r.begin(), r.end());
    }
    if (state_.show.cpu) {
        CpuItem item(state_);
        auto r = item.get();
        all_rows.insert(all_rows.end(), r.begin(), r.end());
    }
    if (state_.show.graphics) {
        GraphicsItem item(state_);
        auto r = item.get();
        all_rows.insert(all_rows.end(), r.begin(), r.end());
    }
    if (state_.show.audio) {
        AudioItem item(state_);
        auto r = item.get();
        all_rows.insert(all_rows.end(), r.begin(), r.end());
    }
    if (state_.show.network) {
        NetworkItem item(state_);
        auto r = item.get();
        all_rows.insert(all_rows.end(), r.begin(), r.end());
    }
    if (state_.show.bluetooth) {
        BluetoothItem item(state_);
        auto r = item.get();
        all_rows.insert(all_rows.end(), r.begin(), r.end());
    }
    if (state_.show.raid) {
        RaidItem item(state_);
        auto r = item.get();
        all_rows.insert(all_rows.end(), r.begin(), r.end());
    }
    if (state_.show.disk) {
        DiskItem item(state_);
        auto r = item.get();
        all_rows.insert(all_rows.end(), r.begin(), r.end());
    }
    if (state_.show.partition) {
        PartitionItem item(state_);
        auto r = item.get();
        all_rows.insert(all_rows.end(), r.begin(), r.end());
    }
    if (state_.show.unmounted) {
        UnmountedItem item(state_);
        auto r = item.get();
        all_rows.insert(all_rows.end(), r.begin(), r.end());
    }
    if (state_.show.usb) {
        UsbItem item(state_);
        auto r = item.get();
        all_rows.insert(all_rows.end(), r.begin(), r.end());
    }
    if (state_.show.sensor) {
        SensorItem item(state_);
        auto r = item.get();
        all_rows.insert(all_rows.end(), r.begin(), r.end());
    }
    if (state_.show.repo) {
        RepoItem item(state_);
        auto r = item.get();
        all_rows.insert(all_rows.end(), r.begin(), r.end());
    }
    if (state_.show.process) {
        ProcessItem item(state_);
        auto r = item.get();
        all_rows.insert(all_rows.end(), r.begin(), r.end());
    }
    if (state_.show.swap) {
        SwapItem item(state_);
        auto r = item.get();
        all_rows.insert(all_rows.end(), r.begin(), r.end());
    }
    if (state_.show.weather) {
        WeatherItem item(state_);
        auto r = item.get();
        all_rows.insert(all_rows.end(), r.begin(), r.end());
    }
    if (state_.show.info) {
        GeneralInfoItem item(state_);
        auto r = item.get();
        all_rows.insert(all_rows.end(), r.begin(), r.end());
    }

    output.handle(all_rows);
}

}  // namespace inxi
