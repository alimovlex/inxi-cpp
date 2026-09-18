// network_item.cpp
#include "items/network_item.h"
#include "utils/file_utils.h"
#include "utils/process_utils.h"
#include "utils/string_utils.h"
#include <filesystem>
#include <map>
#include <sstream>

namespace inxi {

// Build a map from PCI id -> interface name using /sys/class/net/<if>/device
static std::map<std::string, std::string> build_pci_to_if_map() {
    std::map<std::string, std::string> res;
    auto net_dirs = utils::glob_files("/sys/class/net/*");
    for (const auto& d : net_dirs) {
        std::string ifname = std::filesystem::path(d).filename().string();
        if (ifname == "lo") continue;
        // /sys/class/net/<if>/device is a symlink like
        // ../../devices/pci.../0000:01:00.0
        std::string dev_path = d + "/device";
        if (std::filesystem::is_symlink(dev_path)) {
            auto resolved = std::filesystem::read_symlink(dev_path).filename().string();
            res[resolved] = ifname;
        }
    }
    return res;
}

// Read a sys/class/net/<if>/<file> and return trimmed content
static std::string net_read(const std::string& iface, const std::string& file) {
    return utils::trim(utils::read_file_str("/sys/class/net/" + iface + "/" + file));
}

std::vector<NicInfo> NetworkItem::nic_data() {
    std::vector<NicInfo> nics;
    auto pci_to_if = build_pci_to_if_map();

    auto lines = utils::run_capture("lspci -nnk");
    NicInfo cur_nic;
    bool in_net = false;
    int dev_id = 1;
    std::string cur_pci_id;

    for (const auto& line : lines) {
        if (line.empty()) continue;

        // New PCI device
        bool is_new_dev = (line.size() > 4 && std::isxdigit(line[0]) && std::isxdigit(line[1]));
        if (is_new_dev) {
            if (in_net && !cur_nic.model.empty()) {
                nics.push_back(cur_nic);
            }
            in_net = false;
            cur_nic = NicInfo();
            cur_pci_id = "";

            if (line.find("Network controller") != std::string::npos ||
                line.find("Ethernet controller") != std::string::npos) {
                in_net = true;
                cur_nic.if_name = "Device-" + std::to_string(dev_id++);
                // Extract PCI bus id: "01:00.0"
                size_t sp = line.find(' ');
                if (sp != std::string::npos) cur_pci_id = line.substr(0, sp);
                // Extract model name: after last ": "
                size_t colon = line.rfind(": ");
                if (colon != std::string::npos) {
                    std::string model_part = line.substr(colon + 2);
                    // Strip trailing [xxxx:yyyy]
                    size_t brack = model_part.rfind('[');
                    if (brack != std::string::npos) model_part = model_part.substr(0, brack);
                    cur_nic.model = utils::clean(model_part);
                }
            }
        } else if (in_net) {
            if (line.find("Kernel driver in use:") != std::string::npos) {
                cur_nic.driver = utils::trim(line.substr(line.find(':') + 1));
            }
        }
    }
    if (in_net && !cur_nic.model.empty()) {
        nics.push_back(cur_nic);
    }

    // Enrich with /sys/class/net data
    // First build iface name from pci id if possible
    auto net_dirs = utils::glob_files("/sys/class/net/*");
    // Build driver -> iface mapping for supplemental lookup
    std::map<std::string, std::string> driver_to_if;
    for (const auto& d : net_dirs) {
        std::string ifname = std::filesystem::path(d).filename().string();
        if (ifname == "lo") continue;
        std::string drv_path = d + "/device/driver";
        if (std::filesystem::is_symlink(drv_path)) {
            std::string drv = std::filesystem::read_symlink(drv_path).filename().string();
            driver_to_if[drv] = ifname;
        }
        // Also map pci_id -> ifname
        std::string dev_sym = d + "/device";
        if (std::filesystem::is_symlink(dev_sym)) {
            std::string pci = std::filesystem::read_symlink(dev_sym).filename().string();
            pci_to_if[pci] = ifname;
        }
    }

    for (auto& nic : nics) {
        std::string iface;
        // Try to match by PCI id
        for (const auto& [pci, ifc] : pci_to_if) {
            (void)pci;
            (void)ifc;
        }
        // Try driver name first
        if (!nic.driver.empty() && driver_to_if.count(nic.driver)) {
            iface = driver_to_if[nic.driver];
        }
        if (iface.empty()) continue;

        nic.if_name = iface;  // Use real interface name
        nic.mac = net_read(iface, "address");
        std::string operstate = net_read(iface, "operstate");
        nic.up = (operstate == "up");

        // Speed
        std::string speed_str = net_read(iface, "speed");
        if (!speed_str.empty() && utils::is_int(speed_str)) {
            int spd = std::stoi(speed_str);
            if (spd > 0) {
                nic.speed = std::to_string(spd) + " Mbps";
            }
        }

        // Duplex
        nic.duplex = net_read(iface, "duplex");
    }

    // Add USB network adapters via /sys/class/net that aren't already included
    for (const auto& d : net_dirs) {
        std::string ifname = std::filesystem::path(d).filename().string();
        if (ifname == "lo") continue;
        // Check if already in nics
        bool found = false;
        for (const auto& n : nics) {
            if (n.if_name == ifname) { found = true; break; }
        }
        if (found) continue;

        // Check if USB
        std::string dev_sym = d + "/device";
        if (!std::filesystem::exists(dev_sym)) continue;
        std::string resolved_path;
        try {
            resolved_path = std::filesystem::canonical(d + "/device").string();
        } catch (...) { continue; }
        if (resolved_path.find("/usb") == std::string::npos) continue;

        NicInfo nic;
        nic.if_name = ifname;
        nic.mac = net_read(ifname, "address");
        std::string operstate = net_read(ifname, "operstate");
        nic.up = (operstate == "up");
        // USB nic name from device
        std::string prod = utils::trim(utils::read_file_str(resolved_path + "/../product"));
        if (prod.empty()) prod = ifname;
        nic.model = prod;
        nic.port = "USB";
        nics.push_back(nic);
    }

    if (state_.show.ip) {
        ip_data(nics);
    }

    return nics;
}

void NetworkItem::ip_data(std::vector<NicInfo>& nics) {
    auto lines = utils::run_capture("ip -br addr");
    for (const auto& l : lines) {
        std::istringstream iss(l);
        std::string iface, state, addr;
        iss >> iface >> state >> addr;
        for (auto& n : nics) {
            if (n.if_name == iface) {
                size_t slash = addr.find('/');
                if (slash != std::string::npos)
                    n.ip4 = addr.substr(0, slash);
                else
                    n.ip4 = addr;
                break;
            }
        }
    }
}

void NetworkItem::ifconfig_data(std::vector<NicInfo>& nics) {}
void NetworkItem::wireless_data(NicInfo& nic) {}
std::string NetworkItem::wan_data() { return ""; }

std::vector<OutputRow> NetworkItem::short_output(const std::vector<NicInfo>& nics) {
    return full_output(nics);
}

std::vector<OutputRow> NetworkItem::full_output(const std::vector<NicInfo>& nics) {
    std::vector<OutputRow> rows;
    bool is_first = true;
    int dev_id = 1;
    for (const auto& nic : nics) {
        std::vector<Field> fields;
        std::string dev_key = "Device-" + std::to_string(dev_id++);
        fields.push_back(field(dev_key, nic.model));
        if (!nic.driver.empty()) {
            fields.push_back(field("driver", nic.driver));
        }
        if (!nic.port.empty()) {
            fields.push_back(field("type", nic.port));
        }
        if (is_first) {
            rows.push_back(make_row("Network", fields));
            is_first = false;
        } else {
            rows.push_back(make_row("", fields, true));
        }

        // IF line if we have a real interface name
        bool has_real_if = !nic.if_name.empty() &&
                           nic.if_name.find("Device-") == std::string::npos;
        if (has_real_if) {
            std::vector<Field> if_fields;
            if_fields.push_back(field("IF", nic.if_name));
            if_fields.push_back(field("state", nic.up ? "up" : "down"));
            if (!nic.speed.empty() && nic.up) {
                if_fields.push_back(field("speed", nic.speed));
            }
            if (!nic.duplex.empty() && nic.up) {
                if_fields.push_back(field("duplex", nic.duplex));
            }
            if (!nic.mac.empty()) {
                if_fields.push_back(field("mac", filter_data(nic.mac)));
            }
            if (!nic.ip4.empty()) {
                if_fields.push_back(field("ip", filter_data(nic.ip4)));
            }
            rows.push_back(make_row("", if_fields, true));
        }
    }

    if (state_.extra >= 1) {
        std::vector<std::string> services;
        if (!utils::run_capture("pgrep -x NetworkManager").empty()) services.push_back("NetworkManager");
        if (!utils::run_capture("pgrep -x wpa_supplicant").empty()) services.push_back("wpa_supplicant");
        if (!utils::run_capture("pgrep -x iwd").empty()) services.push_back("iwd");
        if (!services.empty()) {
            std::vector<Field> s_fields;
            s_fields.push_back(field("Info", "services: " + utils::join(services, ",")));
            rows.push_back(make_row("", s_fields, true));
        }
    }

    return rows;
}

std::vector<OutputRow> NetworkItem::get() {
    auto nics = nic_data();
    return full_output(nics);
}

}  // namespace inxi
