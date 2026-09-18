// cpu_item.cpp
#include "items/cpu_item.h"
#include "utils/file_utils.h"
#include "utils/process_utils.h"
#include "utils/string_utils.h"
#include <filesystem>
#include <numeric>
#include <regex>
#include <set>
#include <sstream>

namespace inxi {

int CpuItem::clean_speed(const std::string& khz_str) {
    std::string s = utils::trim(khz_str);
    if (utils::is_int(s)) {
        return std::stoi(s) / 1000;
    }
    return 0;
}

std::string CpuItem::prep_short_data(const std::string& model) {
    std::string s = model;
    s = std::regex_replace(s, std::regex(R"(\s*CPU\s*@\s*[\d\.]+\s*[MG]Hz)"), "");
    s = std::regex_replace(s, std::regex(R"(\((R|TM|tm)\))"), "");
    s = std::regex_replace(s, std::regex(R"(\s+)"), " ");
    return utils::trim(s);
}

std::vector<std::string> CpuItem::cpuinfo_data_grabber() {
    return utils::read_file("/proc/cpuinfo");
}

// Read cache sizes from /sys/devices/system/cpu/cpu0/cache/
static void read_cpu_caches(CpuPhysical& cpu) {
    auto index_dirs = utils::glob_files("/sys/devices/system/cpu/cpu0/cache/index*");
    for (const auto& d : index_dirs) {
        std::string level = utils::trim(utils::read_file_str(d + "/level"));
        std::string type  = utils::trim(utils::read_file_str(d + "/type"));
        std::string size  = utils::trim(utils::read_file_str(d + "/size"));
        if (size.empty()) continue;
        std::string key;
        if (level == "1" && type == "Data") key = "l1d";
        else if (level == "1" && type == "Instruction") key = "l1i";
        else if (level == "2" && type != "Instruction") key = "l2";
        else if (level == "3" && type != "Instruction") key = "l3";
        if (!key.empty()) cpu.caches[key] = size;
    }
}

CpuData CpuItem::cpuinfo_data() {
    CpuData data;
    data.arch = state_.cpu_arch;
    data.bits = state_.bits_sys;

    auto lines = cpuinfo_data_grabber();
    CpuPhysical cpu;
    int current_proc = 0;
    std::set<int> core_ids;
    std::set<int> phys_ids;

    for (const auto& line : lines) {
        if (line.rfind("processor", 0) == 0) {
            size_t colon = line.find(':');
            if (colon != std::string::npos)
                current_proc = std::stoi(utils::trim(line.substr(colon + 1)));
        } else if (line.rfind("model name", 0) == 0) {
            size_t colon = line.find(':');
            if (colon != std::string::npos && cpu.model.empty()) {
                cpu.model = prep_short_data(line.substr(colon + 1));
            }
        } else if (line.rfind("vendor_id", 0) == 0) {
            size_t colon = line.find(':');
            if (colon != std::string::npos && cpu.vendor.empty()) {
                cpu.vendor = utils::trim(line.substr(colon + 1));
            }
        } else if (line.rfind("cpu cores", 0) == 0) {
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                cpu.cores = std::stoi(utils::trim(line.substr(colon + 1)));
            }
        } else if (line.rfind("siblings", 0) == 0) {
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                cpu.threads = std::stoi(utils::trim(line.substr(colon + 1)));
            }
        } else if (line.rfind("cpu MHz", 0) == 0) {
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                CpuCore core;
                core.cpu_id = current_proc;
                core.cur_freq_mhz = static_cast<int>(std::stod(utils::trim(line.substr(colon + 1))));
                cpu.logical_cores.push_back(core);
            }
        } else if (line.rfind("core id", 0) == 0) {
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                core_ids.insert(std::stoi(utils::trim(line.substr(colon + 1))));
            }
        } else if (line.rfind("physical id", 0) == 0) {
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                phys_ids.insert(std::stoi(utils::trim(line.substr(colon + 1))));
            }
        }
    }

    if (cpu.cores == 0 && !core_ids.empty()) {
        cpu.cores = static_cast<int>(core_ids.size());
    }
    if (cpu.cores == 0) {
        cpu.cores = static_cast<int>(cpu.logical_cores.size());
    }
    if (cpu.threads == 0) {
        cpu.threads = static_cast<int>(cpu.logical_cores.size());
    }

    // Read /sys/devices/system/cpu/cpu*/cpufreq/
    int min_mhz = 999999, max_mhz = 0;
    auto cpufreq_dirs = utils::glob_files("/sys/devices/system/cpu/cpu[0-9]*/cpufreq");
    for (const auto& cdir : cpufreq_dirs) {
        std::string min_f = utils::trim(utils::read_file_str(cdir + "/cpuinfo_min_freq"));
        std::string max_f = utils::trim(utils::read_file_str(cdir + "/cpuinfo_max_freq"));
        if (!min_f.empty()) {
            int m = clean_speed(min_f);
            if (m > 0 && m < min_mhz) min_mhz = m;
        }
        if (!max_f.empty()) {
            int m = clean_speed(max_f);
            if (m > max_mhz) max_mhz = m;
        }
    }
    if (min_mhz < 999999) cpu.min_freq_mhz = min_mhz;
    if (max_mhz > 0) cpu.max_freq_mhz = max_mhz;

    // Cache info
    read_cpu_caches(cpu);

    data.cpus.push_back(cpu);
    return data;
}

CpuData CpuItem::cpu_sys_data(const std::map<int,int>* sys_freq) {
    return cpuinfo_data();
}

std::string CpuItem::short_data(const CpuData& data) {
    if (data.cpus.empty()) return "";
    return data.cpus[0].model;
}

std::vector<OutputRow> CpuItem::short_output(const CpuData& data) {
    return full_output(data);
}

std::vector<OutputRow> CpuItem::full_output_caches(const CpuPhysical& cpu) {
    return {};
}

std::vector<OutputRow> CpuItem::full_output(const CpuData& data) {
    std::vector<OutputRow> rows;
    if (data.cpus.empty()) return rows;

    const auto& cpu = data.cpus[0];
    std::vector<Field> fields;

    // Core description: "dual core", "quad core", "8-core", etc.
    std::string core_desc;
    if (cpu.cores == 1) core_desc = "single core";
    else if (cpu.cores == 2) core_desc = "dual core";
    else if (cpu.cores == 4) core_desc = "quad core";
    else core_desc = std::to_string(cpu.cores) + "-core";

    std::string mt_type;
    if (cpu.threads > cpu.cores) mt_type = "MT MCP";
    else mt_type = "MCP";

    // "Info: dual core model: Intel Core i3-6100U bits: 64 type: MT MCP cache: L2: 512 KiB"
    fields.push_back(field("Info", core_desc + " model: " + cpu.model));
    fields.push_back(field("bits", std::to_string(data.bits)));
    fields.push_back(field("type", mt_type));
    if (!cpu.caches.empty()) {
        std::string cache_str;
        for (const auto& kv : cpu.caches) {
            if (!cache_str.empty()) cache_str += " ";
            // Format key as L2/L3
            std::string k = kv.first;
            if (k == "l2") k = "L2";
            else if (k == "l3") k = "L3";
            else if (k == "l1d") k = "L1 Data";
            else if (k == "l1i") k = "L1 Inst";
            cache_str += k + ": " + kv.second;
        }
        fields.push_back(field("cache", cache_str));
    }

    rows.push_back(make_row("CPU", fields));

    // Speed line: "Speed (MHz): avg: 500 min/max: 400/2300 cores: 1: 500 2: 500 3: 500 4: 500"
    int avg_speed = 0;
    if (!cpu.logical_cores.empty()) {
        int total_speed = 0;
        for (const auto& c : cpu.logical_cores) total_speed += c.cur_freq_mhz;
        avg_speed = total_speed / static_cast<int>(cpu.logical_cores.size());
    }

    std::vector<Field> spd_fields;
    std::string spd_str = "avg: " + std::to_string(avg_speed);
    if (cpu.min_freq_mhz > 0 && cpu.max_freq_mhz > 0) {
        spd_str += " min/max: " + std::to_string(cpu.min_freq_mhz) + "/" + std::to_string(cpu.max_freq_mhz);
    }
    spd_fields.push_back(field("Speed (MHz)", spd_str));

    // Per-core speeds
    if (!cpu.logical_cores.empty() && cpu.logical_cores.size() <= 32) {
        std::string cores_str = "cores:";
        for (size_t i = 0; i < cpu.logical_cores.size(); ++i) {
            cores_str += " " + std::to_string(i + 1) + ": " + std::to_string(cpu.logical_cores[i].cur_freq_mhz);
        }
        spd_fields.push_back(field("", cores_str));
    }
    rows.push_back(make_row("", spd_fields, true));

    return rows;
}

std::vector<OutputRow> CpuItem::get() {
    auto data = cpuinfo_data();
    return full_output(data);
}

}  // namespace inxi
