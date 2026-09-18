#include "inxi/pci.hpp"

#include "inxi/util.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace inxi {
namespace {

struct IdDb {
    std::unordered_map<std::uint32_t, std::string> vendors;
    std::unordered_map<std::uint64_t, std::string> devices;
};

IdDb load_ids(const std::string& path) {
    IdDb db;
    std::ifstream in(path);
    if (!in) return db;
    std::string line;
    std::uint32_t vendor = 0;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (line[0] != '\t') {
            if (line.size() < 6) continue;
            vendor = static_cast<std::uint32_t>(std::strtoul(line.c_str(), nullptr, 16));
            db.vendors[vendor] = trim(line.substr(4));
        } else if (line.size() > 1 && line[1] != '\t' && vendor) {
            std::uint32_t dev = static_cast<std::uint32_t>(std::strtoul(line.c_str() + 1, nullptr, 16));
            db.devices[(static_cast<std::uint64_t>(vendor) << 16) | dev] = trim(line.substr(6));
        }
    }
    return db;
}

const IdDb& pci_db() {
    static IdDb db = [] {
        for (auto p : {"/usr/share/hwdata/pci.ids", "/usr/share/misc/pci.ids", "/usr/share/pci.ids"}) {
            if (exists(p)) return load_ids(p);
        }
        return IdDb{};
    }();
    return db;
}

const IdDb& usb_db() {
    static IdDb db = [] {
        for (auto p : {"/usr/share/hwdata/usb.ids", "/usr/share/misc/usb.ids", "/usr/share/usb.ids"}) {
            if (exists(p)) return load_ids(p);
        }
        return IdDb{};
    }();
    return db;
}

std::uint32_t hex_sys(const std::string& v) {
    return static_cast<std::uint32_t>(std::strtoul(v.c_str(), nullptr, 16));
}

std::string lookup(const IdDb& db, std::uint32_t ven, std::uint32_t dev, bool vendor) {
    if (vendor) {
        auto it = db.vendors.find(ven);
        return it == db.vendors.end() ? "" : it->second;
    }
    auto it = db.devices.find((static_cast<std::uint64_t>(ven) << 16) | dev);
    return it == db.devices.end() ? "" : it->second;
}

std::string pci_class_name(std::uint32_t cls) {
    auto c = (cls >> 16) & 0xff;
    switch (c) {
        case 0x01: return "Mass storage controller";
        case 0x02: return "Network controller";
        case 0x03: return "VGA compatible controller";
        case 0x04: return "Multimedia controller";
        case 0x06: return "Bridge";
        case 0x07: return "Communication controller";
        case 0x0c: return "USB controller";
        case 0x0d: return "Wireless controller";
        default: {
            char buf[16];
            std::snprintf(buf, sizeof(buf), "%02x%02x", (cls >> 16) & 0xff, (cls >> 8) & 0xff);
            return buf;
        }
    }
}

}  // namespace

const std::vector<PciDevice>& pci_devices() {
    static std::vector<PciDevice> cache;
    static bool loaded = false;
    if (loaded) return cache;
    loaded = true;
    const auto& db = pci_db();
    const std::string root = "/sys/bus/pci/devices";
    for (auto& name : list_dir(root)) {
        PciDevice d;
        d.sysfs = root + "/" + name;
        // 0000:00:02.0
        auto colon = name.find(':');
        d.slot = colon == std::string::npos ? name : name.substr(colon + 1);
        d.vendor_id = hex_sys(read_sysfs(d.sysfs + "/vendor"));
        d.device_id = hex_sys(read_sysfs(d.sysfs + "/device"));
        d.class_id = hex_sys(read_sysfs(d.sysfs + "/class"));
        d.vendor = lookup(db, d.vendor_id, d.device_id, true);
        d.device = lookup(db, d.vendor_id, d.device_id, false);
        if (d.vendor.empty()) {
            char buf[8];
            std::snprintf(buf, sizeof(buf), "%04x", d.vendor_id);
            d.vendor = buf;
        }
        if (d.device.empty()) {
            char buf[8];
            std::snprintf(buf, sizeof(buf), "%04x", d.device_id);
            d.device = buf;
        }
        d.class_name = pci_class_name(d.class_id);
        auto drv = readlink_path(d.sysfs + "/driver");
        d.driver = basename_of(drv);
        cache.push_back(std::move(d));
    }
    return cache;
}

const std::vector<UsbDevice>& usb_devices() {
    static std::vector<UsbDevice> cache;
    static bool loaded = false;
    if (loaded) return cache;
    loaded = true;
    const auto& db = usb_db();
    const std::string root = "/sys/bus/usb/devices";
    for (auto& name : list_dir(root)) {
        if (name.find(':') != std::string::npos) continue;  // interfaces
        if (name.find("usb") == 0 && name.find('.') == std::string::npos &&
            name.find('-') == std::string::npos)
            continue;  // usb hubs roots still useful; skip usb1 bus controllers with no idVendor? 
        auto base = root + "/" + name;
        auto vid = try_read(base + "/idVendor");
        auto pid = try_read(base + "/idProduct");
        if (!vid || !pid) continue;
        UsbDevice d;
        d.bus_port = name;
        d.vendor_id = hex_sys(*vid);
        d.product_id = hex_sys(*pid);
        d.vendor = lookup(db, d.vendor_id, d.product_id, true);
        d.product = lookup(db, d.vendor_id, d.product_id, false);
        if (auto m = try_read(base + "/manufacturer"); m && d.vendor.empty()) d.vendor = *m;
        if (auto p = try_read(base + "/product"); p && d.product.empty()) d.product = *p;
        if (d.vendor.empty()) d.vendor = *vid;
        if (d.product.empty()) d.product = *pid;
        d.cls = read_sysfs(base + "/bDeviceClass");
        d.speed = read_sysfs(base + "/speed");
        // first interface driver
        for (auto& n : list_dir(base)) {
            if (n.find(name + ":") == 0) {
                auto drv = readlink_path(base + "/" + n + "/driver");
                if (!drv.empty()) {
                    d.driver = basename_of(drv);
                    break;
                }
            }
        }
        cache.push_back(std::move(d));
    }
    return cache;
}

std::string pci_id_string(const PciDevice& d) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04x:%04x", d.vendor_id, d.device_id);
    return buf;
}

std::string class_prefix(std::uint32_t class_id, int nibbles) {
    // class is 0xCCSSPP
    unsigned v = (class_id >> 8) & 0xffff;  // class+subclass
    if (nibbles == 2) {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "%02x", (class_id >> 16) & 0xff);
        return buf;
    }
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%04x", v);
    return buf;
}

}  // namespace inxi
