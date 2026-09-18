#include "storage_module.h"
#include "common.h"

#include <filesystem>
#include <vector>
#include <set>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <sys/statvfs.h>

namespace fs = std::filesystem;

namespace mod {
using namespace util;

namespace {

bool shouldSkipBlockDevice(const std::string& name) {
    return startsWith(name, "loop") || startsWith(name, "zram") ||
           startsWith(name, "dm-") || startsWith(name, "md");
}

struct DriveEntry {
    std::string name;
    double bytes = 0;
    std::string model;
    std::string type; // HDD / SSD / NVMe SSD / N/A
};

std::vector<DriveEntry> collectDrives() {
    std::vector<DriveEntry> drives;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator("/sys/block", ec)) {
        std::string name = entry.path().filename().string();
        if (shouldSkipBlockDevice(name)) continue;

        std::string base = entry.path().string();
        std::string sizeStr = readFirstLine(base + "/size");
        double sectors = 0;
        try { sectors = std::stod(sizeStr); } catch (...) {}
        double bytes = sectors * 512.0;
        if (bytes <= 0) continue; // empty optical drive, unused loop slot, etc.

        DriveEntry d;
        d.name = name;
        d.bytes = bytes;
        d.model = readFirstLine(base + "/device/model");
        if (d.model.empty()) d.model = "N/A";

        std::string rota = readFirstLine(base + "/queue/rotational");
        if (startsWith(name, "nvme")) d.type = "NVMe SSD";
        else if (rota == "0") d.type = "SSD";
        else if (rota == "1") d.type = "HDD";
        else d.type = "N/A";

        drives.push_back(d);
    }
    std::sort(drives.begin(), drives.end(), [](auto& a, auto& b) { return a.name < b.name; });
    return drives;
}

struct MountEntry {
    std::string device;
    std::string mountpoint;
    std::string fstype;
};

// /proc/mounts escapes space/tab/newline/backslash as \040 \011 \012 \134.
std::string unescapeMountField(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 3 < s.size() &&
            isdigit((unsigned char)s[i+1]) && isdigit((unsigned char)s[i+2]) && isdigit((unsigned char)s[i+3])) {
            int code = (s[i+1]-'0') * 64 + (s[i+2]-'0') * 8 + (s[i+3]-'0');
            out += static_cast<char>(code);
            i += 3;
        } else {
            out += s[i];
        }
    }
    return out;
}

// Allow-list of "real" filesystem types, rather than trying to blacklist
// every virtual fs (proc, sysfs, cgroup2, fuse.*, squashfs, tmpfs, ...) --
// that list grows every kernel release and this port would always be
// behind it.
bool isRealFilesystem(const std::string& fstype) {
    static const std::set<std::string> real = {
        "ext2", "ext3", "ext4", "xfs", "btrfs", "zfs", "ntfs", "ntfs3",
        "vfat", "fat", "fat32", "exfat", "f2fs", "reiserfs", "reiser4",
        "jfs", "hfs", "hfsplus", "apfs", "udf", "iso9660"
    };
    return real.count(fstype) > 0;
}

std::vector<MountEntry> collectRealMounts() {
    std::vector<MountEntry> mounts;
    for (const auto& line : readLines("/proc/mounts")) {
        std::istringstream ss(line);
        std::string device, mountpoint, fstype;
        if (!(ss >> device >> mountpoint >> fstype)) continue;
        mountpoint = unescapeMountField(mountpoint);
        if (!isRealFilesystem(fstype)) continue;
        mounts.push_back({device, mountpoint, fstype});
    }
    return mounts;
}

struct SpaceInfo {
    double totalBytes = 0;
    double usedBytes = 0;
    double availBytes = 0;
    bool ok = false;
};

// used = blocks - free, matching both `df` and inxi. For the displayed
// percentage we follow inxi's own convention (used / total), not df's
// (used / (used+avail)): those two diverge sharply on any filesystem with
// a project/user quota, where "available" is capped well below the raw
// free space -- exactly the setup in the sandbox this was tested in.
SpaceInfo statMount(const std::string& path) {
    SpaceInfo s;
    struct statvfs vfs{};
    if (statvfs(path.c_str(), &vfs) != 0) return s;
    double frsize = static_cast<double>(vfs.f_frsize ? vfs.f_frsize : vfs.f_bsize);
    s.totalBytes = static_cast<double>(vfs.f_blocks) * frsize;
    s.availBytes = static_cast<double>(vfs.f_bavail) * frsize;
    double freeBytes = static_cast<double>(vfs.f_bfree) * frsize;
    s.usedBytes = s.totalBytes - freeBytes;
    s.ok = true;
    return s;
}

} // namespace

void printDrivesSection(std::ostream& os, bool color) {
    SectionPrinter p(os, color);
    p.header("Drives");

    auto drives = collectDrives();
    if (drives.empty()) {
        p.beginLine();
        p.field("Message", "No drives found under /sys/block.");
        p.endLine();
        return;
    }

    double total = 0;
    for (auto& d : drives) total += d.bytes;
    p.beginLine();
    p.field("Local Storage", "total: " + humanSize(total));
    p.endLine();

    int idx = 1;
    for (auto& d : drives) {
        p.beginLine();
        p.field("ID-" + std::to_string(idx++), "/dev/" + d.name);
        p.field("model", d.model);
        p.field("size", humanSize(d.bytes));
        p.field("type", d.type);
        p.endLine();
    }
}

void printPartitionsSection(std::ostream& os, bool color) {
    SectionPrinter p(os, color);
    p.header("Partition");

    auto mounts = collectRealMounts();
    if (mounts.empty()) {
        p.beginLine();
        p.field("Message", "No mounted partitions with a recognized filesystem were found.");
        p.endLine();
        return;
    }

    int idx = 1;
    for (auto& m : mounts) {
        SpaceInfo s = statMount(m.mountpoint);
        p.beginLine();
        p.field("ID-" + std::to_string(idx++), m.mountpoint);
        if (s.ok) {
            p.field("size", humanSize(s.totalBytes));
            p.field("used", humanSize(s.usedBytes) + " (" + percentStr(s.usedBytes, s.totalBytes) + ")");
        } else {
            p.field("size", "N/A");
        }
        p.field("fs", m.fstype);
        p.field("dev", m.device);
        p.endLine();
    }
}

void printSwapSection(std::ostream& os, bool color) {
    SectionPrinter p(os, color);
    p.header("Swap");

    auto lines = readLines("/proc/swaps");
    bool any = false;
    int idx = 1;
    for (size_t i = 1; i < lines.size(); ++i) { // skip header line
        std::istringstream ss(lines[i]);
        std::string filename, type, sizeKb, usedKb, priority;
        if (!(ss >> filename >> type >> sizeKb >> usedKb >> priority)) continue;
        any = true;
        double sizeBytes = 0, usedBytes = 0;
        try { sizeBytes = std::stod(sizeKb) * 1024.0; } catch (...) {}
        try { usedBytes = std::stod(usedKb) * 1024.0; } catch (...) {}

        p.beginLine();
        p.field("ID-" + std::to_string(idx++), filename);
        p.field("type", type);
        p.field("size", humanSize(sizeBytes));
        p.field("used", humanSize(usedBytes) + " (" + percentStr(usedBytes, sizeBytes) + ")");
        p.endLine();
    }

    if (!any) {
        p.beginLine();
        p.field("Alert", "No swap data was found.");
        p.endLine();
    }
}

double getTotalDriveBytes() {
    double total = 0;
    for (auto& d : collectDrives()) total += d.bytes;
    return total;
}

double getTotalUsedPartitionBytes() {
    double total = 0;
    std::set<std::string> seenDevices;
    for (auto& m : collectRealMounts()) {
        if (!seenDevices.insert(m.device).second) continue; // dedupe bind mounts
        SpaceInfo s = statMount(m.mountpoint);
        if (s.ok) total += s.usedBytes;
    }
    return total;
}

} // namespace mod
