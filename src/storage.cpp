#include "inxi/collect.hpp"

#include "inxi/util.hpp"

#include <cctype>
#include <algorithm>
#include <set>
#include <sys/stat.h>

namespace inxi {
namespace {

bool is_real_disk(const std::string& name) {
    if (starts_with(name, "loop") || starts_with(name, "ram") || starts_with(name, "sr") ||
        starts_with(name, "dm-") || starts_with(name, "zram") || starts_with(name, "md"))
        return false;
    // partitions: sda1, nvme0n1p1
    if (name.size() > 2) {
        auto back = name.back();
        if (std::isdigit(static_cast<unsigned char>(back))) {
            // nvme0n1 is disk; nvme0n1p1 is partition
            if (name.find("nvme") == 0) {
                if (name.find('p') != std::string::npos && name.find('p') > name.find("n"))
                    return false;
            } else if (starts_with(name, "mmcblk")) {
                if (name.find('p') != std::string::npos) return false;
            } else {
                return false;  // sda1
            }
        }
    }
    return true;
}

bool is_optical(const std::string& name) { return starts_with(name, "sr") || starts_with(name, "scd"); }

std::uint64_t block_size(const std::string& name) {
    auto s = read_sysfs("/sys/block/" + name + "/size");  // 512-byte sectors
    return to_u64(s).value_or(0) * 512;
}

std::string disk_model(const std::string& name) {
    auto m = read_sysfs("/sys/block/" + name + "/device/model");
    if (m.empty()) m = read_sysfs("/sys/block/" + name + "/device/name");
    return trim(m);
}

std::string disk_vendor(const std::string& name) {
    return trim(read_sysfs("/sys/block/" + name + "/device/vendor"));
}

std::string disk_rotational(const std::string& name) {
    auto r = read_sysfs("/sys/block/" + name + "/queue/rotational");
    if (r == "0") return "SSD";
    if (r == "1") return "HDD";
    return {};
}

bool skip_fs(const std::string& fs) {
    static const std::set<std::string> skip = {
        "proc", "sysfs", "devtmpfs", "devpts", "tmpfs", "cgroup", "cgroup2", "pstore",
        "bpf", "debugfs", "tracefs", "securityfs", "hugetlbfs", "mqueue", "overlay",
        "autofs", "fusectl", "configfs", "ramfs", "nsfs", "binfmt_misc", "rpc_pipefs",
        "fuse.gvfsd-fuse", "fuse.portal", "efivarfs", "none"};
    return skip.count(fs) || starts_with(fs, "fuse.");
}

}  // namespace

DiskShort disk_summary() {
    DiskShort d;
    for (auto& n : list_dir("/sys/block")) {
        if (!is_real_disk(n)) continue;
        d.total_bytes += block_size(n);
    }
    std::set<std::string> seen_src;
    for (auto& m : parse_mounts()) {
        if (skip_fs(m.fstype)) continue;
        if (m.mountpoint != "/" && !starts_with(m.device, "/dev/")) continue;
        if (seen_src.count(m.device)) continue;
        seen_src.insert(m.device);
        if (auto u = fs_usage(m.mountpoint)) d.used_bytes += u->used;
    }
    return d;
}

void collect_drives(Report& r, const Options& o) {
    Section sec;
    sec.title = "Drives";
    auto sum = disk_summary();
    Row tot;
    add_field(tot, "Local Storage", "total: " + human_size_bytes(sum.total_bytes));
    add_field(tot, "used",
              human_size_bytes(sum.used_bytes) + " (" +
                  percent(static_cast<double>(sum.used_bytes), static_cast<double>(sum.total_bytes)) +
                  ")");
    sec.rows.push_back(std::move(tot));

    if (o.show_disk || o.show_disk_basic) {
        int i = 1;
        for (auto& n : list_dir("/sys/block")) {
            if (!is_real_disk(n) && !(o.show_optical && is_optical(n))) continue;
            if (is_optical(n) && !o.show_optical) continue;
            Row row;
            auto kind = is_optical(n) ? "Optical" : "ID-" + std::to_string(i++);
            add_field(row, kind, n);
            auto vendor = disk_vendor(n);
            auto model = disk_model(n);
            std::string info = trim(vendor + " " + model);
            if (!info.empty()) add_field(row, "model", info);
            add_field(row, "size", human_size_bytes(block_size(n)));
            auto rot = disk_rotational(n);
            if (!rot.empty() && !is_optical(n)) add_field(row, "type", rot);
            if (o.extra >= 1) {
                auto serial = read_sysfs("/sys/block/" + n + "/device/serial");
                if (serial.empty()) serial = read_sysfs("/sys/block/" + n + "/serial");
                if (!serial.empty()) add_field(row, "serial", o.filter ? "<filter>" : serial);
            }
            sec.rows.push_back(std::move(row));
        }
    }
    r.sections.push_back(std::move(sec));
}

void collect_partitions(Report& r, const Options& o) {
    Section sec;
    sec.title = "Partition";
    int i = 1;
    for (auto& m : parse_mounts()) {
        if (skip_fs(m.fstype)) continue;
        if (!starts_with(m.device, "/dev/") && m.mountpoint != "/") continue;
        bool main_only = o.show_partition && !o.show_partition_full;
        if (main_only && m.mountpoint != "/" && m.mountpoint != "/home" && m.mountpoint != "/boot" &&
            m.mountpoint != "/boot/efi" && !starts_with(m.mountpoint, "/usr"))
            continue;
        auto u = fs_usage(m.mountpoint);
        Row row;
        add_field(row, "ID-" + std::to_string(i++), filter_secret(m.mountpoint, o.filter));
        add_field(row, "fs", m.fstype);
        add_field(row, "dev", filter_secret(m.device, o.filter));
        if (u) {
            add_field(row, "size", human_size_bytes(u->total));
            add_field(row, "used", human_size_bytes(u->used) + " (" +
                                       percent(static_cast<double>(u->used), static_cast<double>(u->total)) +
                                       ")");
        }
        if (o.show_label || o.extra >= 1) {
            auto base = basename_of(m.device);
            auto label = read_sysfs("/dev/disk/by-label");  // not a file
            (void)label;
            // try ls
            if (is_dir("/dev/disk/by-label")) {
                for (auto& n : list_dir("/dev/disk/by-label")) {
                    auto tgt = readlink_path("/dev/disk/by-label/" + n);
                    if (basename_of(tgt) == base) {
                        add_field(row, "label", o.filter ? "<filter>" : n);
                        break;
                    }
                }
            }
        }
        if (o.show_uuid) {
            auto base = basename_of(m.device);
            if (is_dir("/dev/disk/by-uuid")) {
                for (auto& n : list_dir("/dev/disk/by-uuid")) {
                    auto tgt = readlink_path("/dev/disk/by-uuid/" + n);
                    if (basename_of(tgt) == base) {
                        add_field(row, "uuid", o.filter ? "<filter>" : n);
                        break;
                    }
                }
            }
        }
        sec.rows.push_back(std::move(row));
    }
    r.sections.push_back(std::move(sec));
}

void collect_swap(Report& r, const Options& o) {
    Section sec;
    sec.title = "Swap";
    auto lines = read_lines("/proc/swaps");
    int i = 1;
    for (size_t n = 1; n < lines.size(); ++n) {
        auto t = split_ws(lines[n]);
        if (t.size() < 5) continue;
        Row row;
        add_field(row, "ID-" + std::to_string(i++), filter_secret(t[0], o.filter));
        add_field(row, "type", t[1]);
        auto size_kib = to_f64(t[2]).value_or(0);
        auto used_kib = to_f64(t[3]).value_or(0);
        add_field(row, "size", human_size(size_kib));
        add_field(row, "used", human_size(used_kib));
        if (o.extra >= 2) add_field(row, "priority", t[4]);
        sec.rows.push_back(std::move(row));
    }
    if (sec.rows.empty()) {
        Row row;
        add_field(row, "Alert", "No swap data found");
        sec.rows.push_back(row);
    }
    r.sections.push_back(std::move(sec));
}

void collect_unmounted(Report& r, const Options& o) {
    (void)o;
    Section sec;
    sec.title = "Unmounted";
    std::set<std::string> mounted;
    for (auto& m : parse_mounts()) mounted.insert(m.device);

    std::set<std::string> parts;
    if (is_dir("/dev/disk/by-uuid")) {
        for (auto& n : list_dir("/sys/block")) {
            auto pdir = "/sys/block/" + n;
            for (auto& sub : list_dir(pdir)) {
                if (!starts_with(sub, n)) continue;
                if (is_dir(pdir + "/" + sub)) parts.insert(sub);
            }
        }
    }
    // also enumerate partition dirs
    for (auto& n : list_dir("/sys/class/block")) {
        if (starts_with(n, "loop") || starts_with(n, "ram") || starts_with(n, "dm-")) continue;
        auto dev = "/dev/" + n;
        struct stat st{};
        if (stat(dev.c_str(), &st) != 0) continue;
        if (!S_ISBLK(st.st_mode)) continue;
        // skip whole disks that have partitions? show partitions not mounted
        bool looks_part = std::isdigit(static_cast<unsigned char>(n.back())) || n.find('p') != std::string::npos;
        if (!looks_part) continue;
        if (mounted.count(dev) || mounted.count("/dev/mapper/" + n)) continue;
        Row row;
        add_field(row, "ID", n);
        add_field(row, "dev", dev);
        auto sz = read_sysfs("/sys/class/block/" + n + "/size");
        auto bytes = to_u64(sz).value_or(0) * 512;
        if (bytes) add_field(row, "size", human_size_bytes(bytes));
        sec.rows.push_back(std::move(row));
    }
    if (sec.rows.empty()) {
        Row row;
        add_field(row, "Message", "No unmounted partitions found");
        sec.rows.push_back(row);
    }
    r.sections.push_back(std::move(sec));
}

void collect_raid(Report& r, const Options& o) {
    (void)o;
    Section sec;
    sec.title = "RAID";
    auto md = read_file("/proc/mdstat");
    bool any = false;
    if (!md.empty() && md.find("md") != std::string::npos) {
        for (auto& line : split(md, '\n')) {
            if (starts_with(line, "md")) {
                any = true;
                Row row;
                auto t = split_ws(line);
                add_field(row, "Device", t.empty() ? line : t[0]);
                if (t.size() > 3) add_field(row, "type", t[3]);
                add_field(row, "raw", trim(line));
                sec.rows.push_back(std::move(row));
            }
        }
    }
    if (is_dir("/sys/class/block")) {
        for (auto& n : list_dir("/sys/class/block")) {
            if (!starts_with(n, "md")) continue;
            any = true;
        }
    }
    if (!any) {
        Row row;
        add_field(row, "Message", "No RAID data found");
        sec.rows.push_back(row);
    }
    r.sections.push_back(std::move(sec));
}

void collect_logical(Report& r, const Options& o) {
    (void)o;
    Section sec;
    sec.title = "Logical";
    int i = 1;
    for (auto& n : list_dir("/sys/class/block")) {
        if (!starts_with(n, "dm-")) continue;
        Row row;
        auto name = read_sysfs("/sys/class/block/" + n + "/dm/name");
        add_field(row, "Device-" + std::to_string(i++), name.empty() ? n : name);
        add_field(row, "dm", n);
        auto sz = to_u64(read_sysfs("/sys/class/block/" + n + "/size")).value_or(0) * 512;
        if (sz) add_field(row, "size", human_size_bytes(sz));
        auto uuid = read_sysfs("/sys/class/block/" + n + "/dm/uuid");
        if (!uuid.empty()) add_field(row, "dm-uuid", uuid);
        sec.rows.push_back(std::move(row));
    }
    if (sec.rows.empty()) {
        Row row;
        add_field(row, "Message", "No logical volume data found");
        sec.rows.push_back(row);
    }
    r.sections.push_back(std::move(sec));
}

}  // namespace inxi
