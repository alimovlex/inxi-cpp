#include "inxi/collect.hpp"

#include "inxi/util.hpp"

#include <unistd.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>
#include <set>
#include <sstream>

namespace inxi {
namespace {

struct LogicalCpu {
    unsigned processor = 0;
    unsigned physical_id = 0;
    unsigned core_id = 0;
    std::string model;
    std::string vendor;
    std::string flags;
    double mhz = 0;
    unsigned cache_kb = 0;
};

std::vector<LogicalCpu> parse_cpuinfo() {
    std::vector<LogicalCpu> cpus;
    LogicalCpu cur;
    bool any = false;
    auto flush = [&] {
        if (any) cpus.push_back(cur);
        cur = {};
        any = false;
    };
    for (auto& line : read_lines("/proc/cpuinfo")) {
        if (line.empty()) {
            flush();
            continue;
        }
        auto p = line.find(':');
        if (p == std::string::npos) continue;
        auto k = trim(line.substr(0, p));
        auto v = trim(line.substr(p + 1));
        any = true;
        if (k == "processor") cur.processor = static_cast<unsigned>(to_u64(v).value_or(0));
        else if (k == "physical id") cur.physical_id = static_cast<unsigned>(to_u64(v).value_or(0));
        else if (k == "core id") cur.core_id = static_cast<unsigned>(to_u64(v).value_or(0));
        else if (k == "model name" || k == "Hardware") cur.model = v;
        else if (k == "vendor_id") cur.vendor = v;
        else if (k == "flags" || k == "Features") cur.flags = v;
        else if (k == "cpu MHz") cur.mhz = to_f64(v).value_or(0);
        else if (k == "cache size") cur.cache_kb = static_cast<unsigned>(to_u64(v).value_or(0));
    }
    flush();
    return cpus;
}

double sysfs_khz(const std::string& path) {
    auto s = read_sysfs(path);
    auto n = to_f64(s);
    if (!n) return 0;
    return *n / 1000.0;  // kHz -> MHz
}

}  // namespace

CpuShort cpu_summary() {
    CpuShort out;
    auto cpus = parse_cpuinfo();
    if (cpus.empty()) {
        out.model = "N/A";
        return out;
    }
    out.model = clean_cpu_model(cpus[0].model);
    out.flags = cpus[0].flags;
    std::set<unsigned> sockets, cores;
    std::map<unsigned, std::set<unsigned>> per_sock;
    for (auto& c : cpus) {
        sockets.insert(c.physical_id);
        per_sock[c.physical_id].insert(c.core_id);
        cores.insert((c.physical_id << 16) | c.core_id);
    }
    out.sockets = static_cast<unsigned>(sockets.size());
    out.cores = static_cast<unsigned>(cores.size());
    out.threads = static_cast<unsigned>(cpus.size());
    out.cores_word = core_count_words(out.cores);
    if (out.sockets > 1) out.cores_word = std::to_string(out.sockets) + "x " + core_count_words(out.cores / out.sockets);

    std::vector<std::string> types;
    if (out.threads > out.cores) types.push_back("MT");
    if (out.cores > 1) types.push_back("MCP");
    if (out.sockets > 1) types.push_back("SMP");
    if (types.empty()) types.push_back("UP");
    out.type = join(types, " ");

    double sum = 0;
    int n = 0;
    double minv = 0, maxv = 0;
    auto cpu_dirs = list_dir("/sys/devices/system/cpu");
    for (auto& d : cpu_dirs) {
        if (!starts_with(d, "cpu") || d.size() < 4 || !std::isdigit(static_cast<unsigned char>(d[3])))
            continue;
        auto base = "/sys/devices/system/cpu/" + d + "/cpufreq/";
        auto cur = sysfs_khz(base + "scaling_cur_freq");
        if (cur <= 0) cur = sysfs_khz(base + "cpuinfo_cur_freq");
        auto mn = sysfs_khz(base + "cpuinfo_min_freq");
        auto mx = sysfs_khz(base + "cpuinfo_max_freq");
        if (cur > 0) {
            sum += cur;
            ++n;
        }
        if (mn > 0 && (minv == 0 || mn < minv)) minv = mn;
        if (mx > 0 && mx > maxv) maxv = mx;
    }
    if (n == 0) {
        for (auto& c : cpus) {
            if (c.mhz > 0) {
                sum += c.mhz;
                ++n;
            }
        }
    }
    out.speed = n ? sum / n : 0;
    out.min = minv;
    out.max = maxv;
    return out;
}

void collect_cpu(Report& r, const Options& o, bool basic) {
    auto c = cpu_summary();
    Section sec;
    sec.title = "CPU";
    Row info;
    std::ostringstream inf;
    inf << c.cores_word << " " << c.model << " [" << c.type << "]";
    add_field(info, "Info", inf.str());

    if (!basic) {
        auto bits = contains_ci(c.flags, " lm") || ends_with(" " + c.flags, " lm") ||
                            c.flags.find("lm") != std::string::npos
                        ? "64"
                        : "32";
        add_field(info, "bits", bits);
        add_field(info, "type", c.type);
        if (o.extra >= 1) {
            add_field(info, "arch", "");  // filled only if we detect
        }
    }
    sec.rows.push_back(std::move(info));

    Row speed;
    auto mhz = [](double v) {
        return std::to_string(static_cast<int>(std::lround(v)));
    };
    add_field(speed, basic ? "speed (MHz)" : "Speed (MHz)", "avg: " + mhz(c.speed));
    if (c.min > 0 || c.max > 0)
        add_field(speed, "min/max", mhz(c.min) + "/" + mhz(c.max));
    if (!basic) {
        std::vector<std::string> per;
        unsigned i = 1;
        for (auto& d : list_dir("/sys/devices/system/cpu")) {
            if (!starts_with(d, "cpu") || d.size() < 4 || !std::isdigit(static_cast<unsigned char>(d[3])))
                continue;
            auto cur = sysfs_khz("/sys/devices/system/cpu/" + d + "/cpufreq/scaling_cur_freq");
            if (cur <= 0) continue;
            per.push_back(std::to_string(i++) + ": " + mhz(cur));
        }
        if (!per.empty() && o.extra >= 0) add_field(speed, "cores", join(per, " "));
    }
    sec.rows.push_back(std::move(speed));

    if (o.show_cpu_flag && !c.flags.empty()) {
        Row f;
        add_field(f, "Flags", c.flags);
        sec.rows.push_back(std::move(f));
    }

    if (o.admin) {
        Row vul;
        std::vector<std::string> items;
        std::string vdir = "/sys/devices/system/cpu/vulnerabilities";
        if (is_dir(vdir)) {
            for (auto& n : list_dir(vdir)) {
                auto val = read_sysfs(vdir + "/" + n);
                if (val.empty()) continue;
                if (o.filter && val.find("Vulnerable") == std::string::npos &&
                    val.find("vulnerable") == std::string::npos)
                    continue;
                items.push_back(n + ": " + val);
            }
        }
        if (!items.empty()) {
            add_field(vul, "Vulnerabilities", join(items, "; "));
            sec.rows.push_back(std::move(vul));
        }
    }

    r.sections.push_back(std::move(sec));
}

void collect_short(Report& r, const Options& o) {
    auto c = cpu_summary();
    auto mem = mem_summary();
    auto disk = disk_summary();
    Section sec;
    Row row;
    std::string type = c.type.empty() ? "" : " (-" + c.type + "-)";
    add_field(row, "CPU", c.cores_word + " " + c.model + type);
    auto mhz = [](double v) { return std::to_string(static_cast<int>(std::lround(v))); };
    if (c.min > 0 && c.max > 0)
        add_field(row, "speed/min/max", mhz(c.speed) + "/" + mhz(c.min) + "/" + mhz(c.max) + " MHz");
    else
        add_field(row, "speed", mhz(c.speed) + " MHz");

    auto rel = trim(read_sysfs("/proc/sys/kernel/osrelease"));
    if (rel.empty()) rel = trim(exec("uname -r"));
    auto arch = trim(exec("uname -m"));
    add_field(row, "Kernel", trim(rel + " " + arch));

    auto up = to_f64(split_ws(read_file("/proc/uptime"))[0]);
    add_field(row, "Up", format_uptime(up.value_or(0)));

    auto used = human_size(mem.used_kib);
    auto tot = human_size(mem.total_kib);
    auto pct = percent(mem.used_kib, mem.total_kib);
    add_field(row, "Mem", used + "/" + tot + " (" + pct + ")");

    std::string stor = "N/A";
    if (disk.total_bytes) {
        stor = human_size_bytes(disk.total_bytes);
        if (disk.used_bytes)
            stor += " (" + percent(static_cast<double>(disk.used_bytes),
                                   static_cast<double>(disk.total_bytes)) +
                    " used)";
    }
    add_field(row, "Storage", stor);
    add_field(row, "Procs", std::to_string(process_count()));

    pid_t p = parent_pid(getpid());
    std::string shell = proc_comm(p);
    // skip wrappers
    for (int i = 0; i < 6 && (shell == "inxi" || shell == "timeout" || shell == "sh" ||
                              shell == "bash" && proc_cmdline(p).find("inxi") != std::string::npos);
         ++i) {
        auto pp = parent_pid(p);
        if (!pp || pp == p) break;
        p = pp;
        shell = proc_comm(p);
    }
    add_field(row, "Client", shell.empty() ? "N/A" : shell);
    add_field(row, "inxi", INXI_CPP_VERSION);
    sec.rows.push_back(std::move(row));
    r.sections.push_back(std::move(sec));
    (void)o;
}

}  // namespace inxi
