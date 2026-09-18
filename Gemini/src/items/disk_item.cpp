// disk_item.cpp
#include "items/disk_item.h"
#include "utils/file_utils.h"
#include "utils/math_utils.h"
#include "utils/process_utils.h"
#include "utils/string_utils.h"
#include <set>
#include <sstream>
#include <sys/statvfs.h>
#include <filesystem>

namespace inxi {

bool DiskItem::rotational_data(const std::string& dev_name) {
    std::string path = "/sys/block/" + dev_name + "/queue/rotational";
    std::string val = utils::read_file_str(path);
    return (val == "1");
}

std::string DiskItem::transport_data(const std::string& dev_path) {
    return "";
}

std::string DiskItem::disk_vendor(const DiskEntry& d) {
    return d.vendor;
}

void DiskItem::smart_data(DiskEntry& d) {}

std::vector<DiskEntry> DiskItem::disk_data_bsd() { return {}; }

std::vector<DiskEntry> DiskItem::lsblk_data() {
    std::vector<DiskEntry> disks;
    auto lines = utils::run_capture("lsblk -b -d -P -o NAME,SIZE,TYPE,VENDOR,MODEL,TRAN,ROTA,SERIAL");
    if (lines.empty()) {
        auto block_dirs = utils::glob_files("/sys/block/*");
        for (const auto& bd : block_dirs) {
            std::string name = std::filesystem::path(bd).filename().string();
            if (name.rfind("loop", 0) == 0 || name.rfind("ram", 0) == 0) continue;
            std::string size_sectors = utils::read_file_str(bd + "/size");
            if (!size_sectors.empty() && utils::is_int(size_sectors)) {
                uint64_t bytes = std::stoull(size_sectors) * 512ULL;
                if (bytes == 0) continue;
                DiskEntry d;
                d.name = name;
                d.path = "/dev/" + name;
                d.size_bytes = bytes;
                d.size = utils::format_bytes(bytes, 2);
                d.model = utils::clean_disk(utils::read_file_str(bd + "/device/model"));
                d.rotational = rotational_data(name);
                if (utils::to_lower(d.model).find("ssd") != std::string::npos || d.name.rfind("nvme", 0) == 0) {
                    d.rotational = false;
                }
                d.type = d.rotational ? "HDD" : "SSD";
                disks.push_back(d);
            }
        }
        return disks;
    }

    for (const auto& line : lines) {
        // Parse KEY="VALUE"
        std::map<std::string, std::string> kv;
        size_t i = 0;
        while (i < line.size()) {
            size_t eq = line.find('=', i);
            if (eq == std::string::npos) break;
            std::string key = line.substr(i, eq - i);
            key = utils::trim(key);
            if (eq + 1 < line.size() && line[eq + 1] == '"') {
                size_t qend = line.find('"', eq + 2);
                if (qend != std::string::npos) {
                    std::string val = line.substr(eq + 2, qend - (eq + 2));
                    kv[key] = val;
                    i = qend + 1;
                } else {
                    break;
                }
            } else {
                break;
            }
        }

        if (kv["TYPE"] != "disk") continue;

        DiskEntry d;
        d.name = kv["NAME"];
        d.path = "/dev/" + d.name;
        if (utils::is_int(kv["SIZE"])) {
            d.size_bytes = std::stoull(kv["SIZE"]);
            d.size = utils::format_bytes(d.size_bytes, 2);
        }
        d.vendor = utils::clean(utils::trim(kv["VENDOR"]));
        d.model = utils::clean_disk(utils::trim(kv["MODEL"]));
        d.transport = utils::trim(kv["TRAN"]);
        d.serial = utils::trim(kv["SERIAL"]);
        d.rotational = (kv["ROTA"] == "1");

        // Vendor inference if empty or "ATA"
        if (d.vendor.empty() || d.vendor == "ATA") {
            if (d.model.rfind("CT", 0) == 0 || d.model.find("Crucial") != std::string::npos) {
                d.vendor = "Crucial";
            } else if (d.model.rfind("WDC", 0) == 0 || d.model.rfind("WD", 0) == 0) {
                d.vendor = "WDC";
            } else if (d.model.rfind("ST", 0) == 0 || d.model.find("Seagate") != std::string::npos) {
                d.vendor = "Seagate";
            } else if (d.model.find("Samsung") != std::string::npos) {
                d.vendor = "Samsung";
            } else if (d.model.find("Kingston") != std::string::npos) {
                d.vendor = "Kingston";
            } else if (d.model.find("SanDisk") != std::string::npos) {
                d.vendor = "SanDisk";
            }
        }

        // SSD check
        if (utils::to_lower(d.model).find("ssd") != std::string::npos || d.name.rfind("nvme", 0) == 0) {
            d.rotational = false;
        }
        d.type = d.rotational ? "HDD" : "SSD";

        disks.push_back(d);
    }

    return disks;
}

std::vector<OutputRow> DiskItem::short_output(const std::vector<DiskEntry>& disks) {
    std::vector<OutputRow> rows;
    uint64_t total_storage = 0;
    for (const auto& d : disks) {
        total_storage += d.size_bytes;
    }

    // Calculate used storage across mounted filesystems
    uint64_t used_storage = 0;
    std::set<std::string> seen_devs;
    auto mount_lines = utils::read_file("/proc/mounts");
    for (const auto& line : mount_lines) {
        std::string dev = utils::awk_field(line, 1);
        std::string mnt = utils::awk_field(line, 2);
        std::string fstype = utils::awk_field(line, 3);
        if (dev.rfind("/dev/", 0) == 0 && fstype != "tmpfs" && fstype != "devtmpfs" && fstype != "overlay") {
            if (seen_devs.insert(dev).second) {
                struct statvfs st;
                if (statvfs(mnt.c_str(), &st) == 0) {
                    uint64_t bsize = st.f_frsize ? st.f_frsize : st.f_bsize;
                    uint64_t total_b = st.f_blocks * bsize;
                    uint64_t free_b = st.f_bfree * bsize;
                    if (total_b > free_b) {
                        used_storage += (total_b - free_b);
                    }
                }
            }
        }
    }

    std::vector<Field> fields;
    std::string total_h = utils::format_bytes(total_storage, 2);
    std::string used_h = utils::format_bytes(used_storage, 2);
    std::string pct = utils::format_percent(static_cast<double>(used_storage), static_cast<double>(total_storage));
    fields.push_back(field("Local Storage", "total: " + total_h + " used: " + used_h + " (" + pct + ")"));
    rows.push_back(make_row("Drives", fields));

    if (state_.show.disk_total || state_.show.disk) {
        int id = 1;
        for (const auto& d : disks) {
            std::vector<Field> df;
            df.push_back(field("ID-" + std::to_string(id++), d.path));
            if (!d.vendor.empty()) df.push_back(field("vendor", d.vendor));
            if (!d.model.empty()) df.push_back(field("model", d.model));
            df.push_back(field("size", d.size));
            if (d.transport == "usb") {
                df.push_back(field("type", "USB"));
                df.push_back(field("tech", d.type));
            } else {
                df.push_back(field("type", d.type));
            }
            if (state_.extra >= 1 && !d.serial.empty()) {
                df.push_back(field("serial", filter_data(d.serial)));
            }
            rows.push_back(make_row("", df, true));
        }
    }

    return rows;
}

std::vector<OutputRow> DiskItem::full_output(const std::vector<DiskEntry>& disks) {
    return short_output(disks);
}

std::vector<OutputRow> DiskItem::get() {
    auto disks = lsblk_data();
    return short_output(disks);
}

}  // namespace inxi
