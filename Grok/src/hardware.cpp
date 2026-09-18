#include "inxi/collect.hpp"

#include "inxi/pci.hpp"
#include "inxi/util.hpp"

#include <cctype>
#include <cstdio>
#include <algorithm>
#include <map>
#include <set>
#include <sstream>

namespace inxi {
namespace {

std::string chassis_type(const std::string& code) {
    auto n = to_u64(code).value_or(0);
    switch (n) {
        case 3: return "Desktop";
        case 4: return "Low Profile Desktop";
        case 6: return "Mini Tower";
        case 7: return "Tower";
        case 8: return "Portable";
        case 9: return "Laptop";
        case 10: return "Notebook";
        case 11: return "Hand Held";
        case 14: return "Sub Notebook";
        case 30: return "Tablet";
        case 31: return "Convertible";
        default: return n ? "Other" : "";
    }
}

std::string dmi(const std::string& key) {
    return read_sysfs("/sys/class/dmi/id/" + key);
}

bool is_display(const PciDevice& d) {
    return ((d.class_id >> 16) & 0xff) == 0x03;
}
bool is_audio(const PciDevice& d) {
    auto c = (d.class_id >> 16) & 0xff;
    auto s = (d.class_id >> 8) & 0xff;
    return c == 0x04 && (s == 0x01 || s == 0x03);
}
bool is_net(const PciDevice& d) {
    auto c = (d.class_id >> 16) & 0xff;
    return c == 0x02;
}
bool is_bt_pci(const PciDevice& d) {
    auto c = (d.class_id >> 16) & 0xff;
    auto s = (d.class_id >> 8) & 0xff;
    return c == 0x0d && s == 0x11;
}

void add_device_row(Section& sec, int idx, const std::string& name, const std::string& driver,
                    const Options& o, const std::string& chip_id, const std::string& extra_type = {}) {
    Row row;
    add_field(row, "Device-" + std::to_string(idx), name);
    if (!driver.empty()) add_field(row, "driver", driver);
    if (driver == "i915" || driver == "amdgpu" || driver == "nouveau" || driver == "radeon")
        add_field(row, "v", "kernel");
    if (!extra_type.empty()) add_field(row, "type", extra_type);
    if (o.extra >= 2 && !chip_id.empty()) add_field(row, "chip-ID", chip_id);
    sec.rows.push_back(std::move(row));
}

std::string iface_mac(const std::string& ifn, bool filter) {
    auto mac = read_sysfs("/sys/class/net/" + ifn + "/address");
    if (filter && mac.size() >= 8) return "<filter>";
    return mac;
}

std::string iface_ip(const std::string& ifn, bool filter) {
    auto out = exec("ip -o -4 addr show dev " + ifn + " 2>/dev/null");
    if (out.empty()) out = exec("ifconfig " + ifn + " 2>/dev/null");
    std::string ip;
    auto toks = split_ws(out);
    for (size_t i = 0; i + 1 < toks.size(); ++i) {
        if (toks[i] == "inet") {
            ip = toks[i + 1];
            auto sl = ip.find('/');
            if (sl != std::string::npos) ip = ip.substr(0, sl);
            break;
        }
    }
    if (filter && !ip.empty()) return "<filter>";
    return ip;
}

std::string gl_renderer() {
    auto p = which("glxinfo");
    if (p.empty()) return {};
    auto out = exec(p + " -B 2>/dev/null");
    std::string vendor, ver, renderer;
    for (auto& line : split(out, '\n')) {
        if (starts_with(line, "OpenGL vendor string:")) vendor = trim(line.substr(22));
        else if (starts_with(line, "OpenGL version string:")) ver = trim(line.substr(23));
        else if (starts_with(line, "OpenGL renderer string:")) renderer = trim(line.substr(24));
    }
    if (ver.empty() && renderer.empty()) return {};
    // version first token
    auto vtok = split_ws(ver);
    std::string v = vtok.empty() ? ver : vtok[0];
    std::string s = v;
    if (!vendor.empty()) s += " vendor: " + to_lower(vendor);
    if (!renderer.empty()) s += " renderer: " + renderer;
    return s;
}

std::string display_server() {
    if (!env("WAYLAND_DISPLAY").empty()) return "wayland";
    if (!env("DISPLAY").empty()) return "x11";
    return {};
}

}  // namespace

void collect_machine(Report& r, const Options& o) {
    Section sec;
    sec.title = "Machine";
    Row row;
    auto type = chassis_type(dmi("chassis_type"));
    if (type.empty()) type = "N/A";
    add_field(row, "Type", type);
    auto sys = dmi("sys_vendor");
    auto product = dmi("product_name");
    auto pver = dmi("product_version");
    add_field(row, "System", sys.empty() ? "N/A" : sys);
    add_field(row, "product", product.empty() ? "N/A" : product);
    add_field(row, "v", pver.empty() ? "N/A" : pver);
    if (!o.filter) {
        auto ser = dmi("product_serial");
        add_field(row, "serial", ser.empty() ? "N/A" : ser);
    } else {
        add_field(row, "serial", "<filter>");
    }
    sec.rows.push_back(std::move(row));

    Row board;
    add_field(board, "Mobo", dmi("board_vendor").empty() ? "N/A" : dmi("board_vendor"));
    add_field(board, "model", dmi("board_name").empty() ? "N/A" : dmi("board_name"));
    add_field(board, "v", dmi("board_version").empty() ? "N/A" : dmi("board_version"));
    if (o.filter) add_field(board, "serial", "<filter>");
    else {
        auto s = dmi("board_serial");
        add_field(board, "serial", s.empty() ? "N/A" : s);
    }
    auto bios_v = dmi("bios_vendor");
    auto bios_ver = dmi("bios_version");
    auto bios_date = dmi("bios_date");
    add_field(board, "Firmware", exists("/sys/firmware/efi") ? "UEFI" : "BIOS");
    add_field(board, "vendor", bios_v.empty() ? "N/A" : bios_v);
    add_field(board, "v", bios_ver.empty() ? "N/A" : bios_ver);
    add_field(board, "date", bios_date.empty() ? "N/A" : bios_date);
    if (o.extra >= 2) {
        auto ch = dmi("chassis_vendor");
        if (!ch.empty()) add_field(board, "chassis", ch);
    }
    sec.rows.push_back(std::move(board));
    r.sections.push_back(std::move(sec));
}

void collect_battery(Report& r, const Options& o) {
    Section sec;
    sec.title = "Battery";
    int id = 1;
    bool any = false;
    for (auto& n : list_dir("/sys/class/power_supply")) {
        auto base = "/sys/class/power_supply/" + n;
        auto type = to_lower(read_sysfs(base + "/type"));
        if (type != "battery") continue;
        any = true;
        Row row;
        add_field(row, "ID-" + std::to_string(id++), n);
        auto energy_now = to_f64(read_sysfs(base + "/energy_now")).value_or(0);
        auto energy_full = to_f64(read_sysfs(base + "/energy_full")).value_or(0);
        auto energy_design = to_f64(read_sysfs(base + "/energy_full_design")).value_or(0);
        auto charge_now = to_f64(read_sysfs(base + "/charge_now")).value_or(0);
        auto charge_full = to_f64(read_sysfs(base + "/charge_full")).value_or(0);
        auto charge_design = to_f64(read_sysfs(base + "/charge_full_design")).value_or(0);
        // micro-Wh or micro-Ah
        double now = energy_now > 0 ? energy_now / 1e6 : charge_now / 1e6;
        double full = energy_full > 0 ? energy_full / 1e6 : charge_full / 1e6;
        double design = energy_design > 0 ? energy_design / 1e6 : charge_design / 1e6;
        auto fmt = [](double v) {
            std::ostringstream ss;
            ss.setf(std::ios::fixed);
            ss.precision(v >= 10 ? 1 : 2);
            ss << v;
            return ss.str();
        };
        if (full > 0) {
            add_field(row, "charge", fmt(now) + " Wh (" + percent(now, full) + ")");
            if (design > 0)
                add_field(row, "condition",
                          fmt(full) + "/" + fmt(design) + " Wh (" + percent(full, design) + ")");
        } else {
            auto cap = read_sysfs(base + "/capacity");
            add_field(row, "charge", cap.empty() ? "N/A" : cap + "%");
        }
        auto status = read_sysfs(base + "/status");
        if (o.extra >= 1 && !status.empty()) add_field(row, "status", to_lower(status));
        if (o.extra >= 1) {
            auto vnow = to_f64(read_sysfs(base + "/voltage_now")).value_or(0);
            auto vmin = to_f64(read_sysfs(base + "/voltage_min_design")).value_or(0);
            if (vnow > 0)
                add_field(row, "volts", fmt(vnow / 1e6) + (vmin ? " min: " + fmt(vmin / 1e6) : ""));
        }
        if (o.extra >= 2 && !o.filter) {
            auto ser = read_sysfs(base + "/serial_number");
            if (!ser.empty()) add_field(row, "serial", ser);
        }
        sec.rows.push_back(std::move(row));
    }
    if (!any && o.show_battery_forced) {
        Row row;
        add_field(row, "Message", "No system battery found");
        sec.rows.push_back(row);
    }
    if (!sec.rows.empty()) r.sections.push_back(std::move(sec));
}

void collect_graphics(Report& r, const Options& o) {
    Section sec;
    sec.title = "Graphics";
    int i = 1;
    for (auto& d : pci_devices()) {
        if (!is_display(d)) continue;
        add_device_row(sec, i++, d.vendor + " " + d.device, d.driver, o, pci_id_string(d));
    }
    for (auto& u : usb_devices()) {
        auto cls = to_u64(u.cls).value_or(0);
        bool cam = cls == 0x0e || contains_ci(u.product, "camera") || contains_ci(u.product, "webcam") ||
                   u.driver == "uvcvideo";
        if (!cam) continue;
        add_device_row(sec, i++, u.vendor + " " + u.product, u.driver, o, "", "USB");
    }

    Row disp;
    auto server = display_server();
    if (!server.empty()) {
        add_field(disp, "Display", server);
        if (server == "x11") {
            add_field(disp, "server", "X.Org");
            auto xv = exec("xdpyinfo 2>/dev/null | awk '/version number/ {print $3}'");
            // better: Xorg -version is noisy. Try xdpyinfo
            auto info = exec("xdpyinfo 2>/dev/null");
            std::string ver;
            for (auto& line : split(info, '\n')) {
                if (line.find("version number") != std::string::npos) {
                    auto t = split_ws(line);
                    if (!t.empty()) ver = t.back();
                }
            }
            if (ver.empty()) {
                auto logv = exec("Xorg -version 2>&1");
                for (auto& line : split(logv, '\n')) {
                    if (line.find("X.Org X Server") != std::string::npos) {
                        auto t = split_ws(line);
                        if (!t.empty()) ver = t.back();
                    }
                }
            }
            if (!ver.empty()) add_field(disp, "v", ver);
        } else {
            add_field(disp, "server", "Wayland");
        }
        // loaded drm driver
        std::vector<std::string> gpus;
        for (auto& d : pci_devices())
            if (is_display(d) && !d.driver.empty()) gpus.push_back(d.driver);
        if (!gpus.empty()) add_field(disp, "gpu", join(gpus, ","));
        if (which("xrandr").size() && !env("DISPLAY").empty()) {
            auto xr = exec("xrandr --current 2>/dev/null");
            std::vector<std::string> res;
            int n = 1;
            for (auto& line : split(xr, '\n')) {
                if (line.find(" connected") != std::string::npos) {
                    auto t = split_ws(line);
                    std::string rstr = "N/A";
                    for (auto& tok : t)
                        if (tok.find('x') != std::string::npos && std::isdigit(static_cast<unsigned char>(tok[0]))) {
                            rstr = tok;
                            auto plus = rstr.find('+');
                            if (plus != std::string::npos) rstr = rstr.substr(0, plus);
                            break;
                        }
                    res.push_back(std::to_string(n++) + ": " + rstr);
                }
            }
            if (!res.empty()) add_field(disp, "resolution", join(res, " "));
        }
        sec.rows.push_back(std::move(disp));
    }

    auto gl = gl_renderer();
    if (!gl.empty()) {
        Row api;
        // gl_renderer returns "ver vendor: ... renderer: ..."
        auto toks = split_ws(gl);
        add_field(api, "API", "OpenGL");
        if (!toks.empty()) add_field(api, "v", toks[0]);
        auto vp = gl.find("vendor:");
        auto rp = gl.find("renderer:");
        if (vp != std::string::npos) {
            auto vend = gl.substr(vp + 7, rp == std::string::npos ? std::string::npos : rp - vp - 7);
            add_field(api, "vendor", trim(vend));
        }
        if (rp != std::string::npos) add_field(api, "renderer", trim(gl.substr(rp + 9)));
        sec.rows.push_back(std::move(api));
    }

    if (o.extra >= 1 || o.show_graphic_full) {
        Row tools;
        std::vector<std::string> api;
        if (!which("eglinfo").empty()) api.push_back("eglinfo");
        if (!which("glxinfo").empty()) api.push_back("glxinfo");
        if (!which("vulkaninfo").empty()) api.push_back("vulkaninfo");
        add_field(tools, "Info", "");
        if (!api.empty()) add_field(tools, "Tools", "api: " + join(api, ", "));
        sec.rows.push_back(std::move(tools));
    }
    r.sections.push_back(std::move(sec));
}

void collect_audio(Report& r, const Options& o) {
    Section sec;
    sec.title = "Audio";
    int i = 1;
    for (auto& d : pci_devices()) {
        if (!is_audio(d)) continue;
        add_device_row(sec, i++, d.vendor + " " + d.device, d.driver, o, pci_id_string(d));
    }
    for (auto& u : usb_devices()) {
        auto cls = to_u64(u.cls).value_or(0);
        if (cls != 0x01 && u.driver.find("snd") == std::string::npos &&
            !contains_ci(u.product, "audio") && !contains_ci(u.product, "headset"))
            continue;
        if (cls != 0x01 && u.driver.find("snd") == std::string::npos) continue;
        add_device_row(sec, i++, u.vendor + " " + u.product, u.driver, o, "", "USB");
    }
    Row api;
    if (is_dir("/proc/asound") || is_dir("/sys/class/sound")) {
        add_field(api, "API", "ALSA");
        auto ver = read_sysfs("/proc/asound/version");
        if (!ver.empty()) {
            auto t = split_ws(ver);
            if (t.size() >= 2) add_field(api, "v", t.back());
        }
        add_field(api, "status", "kernel-api");
        sec.rows.push_back(api);
    }
    int s = 1;
    auto has_proc = [](const std::string& name) {
        for (auto& n : list_dir("/proc")) {
            if (!n.empty() && std::isdigit(static_cast<unsigned char>(n[0])) && proc_comm(static_cast<pid_t>(to_u64(n).value_or(0))) == name)
                return true;
        }
        return false;
    };
    auto add_server = [&](const std::string& label, const std::string& pname, const std::string& vcmd) {
        if (!has_proc(pname) && which(pname).empty()) return;
        if (!has_proc(pname)) return;
        Row row;
        add_field(row, "Server-" + std::to_string(s++), label);
        auto ver = trim(exec(vcmd));
        auto t = split_ws(split(ver, '\n')[0]);
        for (auto& tok : t)
            if (!tok.empty() && std::isdigit(static_cast<unsigned char>(tok[0])) && tok.find('.') != std::string::npos) {
                add_field(row, "v", tok);
                break;
            }
        add_field(row, "status", "active");
        sec.rows.push_back(std::move(row));
    };
    add_server("PipeWire", "pipewire", "pipewire --version 2>/dev/null");
    add_server("PulseAudio", "pulseaudio", "pulseaudio --version 2>/dev/null");
    add_server("JACK", "jackd", "jackd --version 2>/dev/null");
    if (sec.rows.empty()) {
        Row row;
        add_field(row, "Message", "No audio devices found");
        sec.rows.push_back(row);
    }
    r.sections.push_back(std::move(sec));
}

void collect_network(Report& r, const Options& o) {
    Section sec;
    sec.title = "Network";
    int i = 1;
    for (auto& d : pci_devices()) {
        if (!is_net(d)) continue;
        add_device_row(sec, i++, d.vendor + " " + d.device, d.driver, o, pci_id_string(d));
        if (o.show_network_advanced) {
            // match iface by driver
            for (auto& n : list_dir("/sys/class/net")) {
                if (n == "lo") continue;
                auto drv = basename_of(readlink_path("/sys/class/net/" + n + "/device/driver"));
                auto slot_link = readlink_path("/sys/class/net/" + n + "/device");
                if (slot_link.find(d.slot) == std::string::npos && drv != d.driver) continue;
                if (slot_link.find(d.slot) == std::string::npos && !drv.empty() && drv != d.driver)
                    continue;
                if (slot_link.find(d.slot) == std::string::npos) continue;
                Row ifr;
                add_field(ifr, "IF", n);
                auto state = read_sysfs("/sys/class/net/" + n + "/operstate");
                add_field(ifr, "state", state.empty() ? "N/A" : state);
                auto mac = iface_mac(n, o.filter);
                if (!mac.empty()) add_field(ifr, "mac", mac);
                if (o.show_ip) {
                    auto ip = iface_ip(n, o.filter);
                    if (!ip.empty()) add_field(ifr, "ipv4", ip);
                }
                sec.rows.push_back(std::move(ifr));
            }
        }
    }
    for (auto& u : usb_devices()) {
        if (u.driver != "cdc_ether" && u.driver != "rndis_host" && u.driver != "asix" &&
            !contains_ci(u.product, "ethernet") && !contains_ci(u.product, "wifi") &&
            !contains_ci(u.product, "wlan") && u.driver != "rt2800usb")
            continue;
        add_device_row(sec, i++, u.vendor + " " + u.product, u.driver, o, "", "USB");
    }
    r.sections.push_back(std::move(sec));
}

void collect_bluetooth(Report& r, const Options& o) {
    Section sec;
    sec.title = "Bluetooth";
    int i = 1;
    bool any = false;
    for (auto& d : pci_devices()) {
        if (!is_bt_pci(d)) continue;
        any = true;
        add_device_row(sec, i++, d.vendor + " " + d.device, d.driver, o, pci_id_string(d));
    }
    for (auto& u : usb_devices()) {
        auto cls = to_u64(u.cls).value_or(0);
        if (cls != 0xe0 && !contains_ci(u.product, "bluetooth") && u.driver != "btusb") continue;
        any = true;
        add_device_row(sec, i++, u.vendor + " " + u.product, u.driver, o, "", "USB");
    }
    if (is_dir("/sys/class/bluetooth")) {
        for (auto& n : list_dir("/sys/class/bluetooth")) {
            Row row;
            add_field(row, "Report", n);
            auto addr = read_sysfs("/sys/class/bluetooth/" + n + "/address");
            if (!addr.empty()) add_field(row, "bt-id", o.filter ? "<filter>" : addr);
            sec.rows.push_back(std::move(row));
            any = true;
        }
    }
    if (!any && o.show_bluetooth_forced) {
        Row row;
        add_field(row, "Message", "No bluetooth data found");
        sec.rows.push_back(row);
    }
    if (!sec.rows.empty()) r.sections.push_back(std::move(sec));
}

void collect_usb(Report& r, const Options& o) {
    Section sec;
    sec.title = "USB";
    int i = 1;
    for (auto& u : usb_devices()) {
        Row row;
        add_field(row, "Hub-" + std::to_string(i++), u.bus_port);
        add_field(row, "info", u.vendor + " " + u.product);
        if (!u.driver.empty()) add_field(row, "driver", u.driver);
        if (!u.speed.empty()) add_field(row, "speed", u.speed + " Mb/s");
        if (o.extra >= 2) {
            char buf[16];
            std::snprintf(buf, sizeof(buf), "%04x:%04x", u.vendor_id, u.product_id);
            add_field(row, "chip-ID", buf);
        }
        sec.rows.push_back(std::move(row));
    }
    if (sec.rows.empty()) {
        Row row;
        add_field(row, "Message", "No USB devices found");
        sec.rows.push_back(row);
    }
    r.sections.push_back(std::move(sec));
}

void collect_sensors(Report& r, const Options& o) {
    (void)o;
    Section sec;
    sec.title = "Sensors";
    std::vector<std::string> temps, fans, volts;
    if (is_dir("/sys/class/hwmon")) {
        for (auto& n : list_dir("/sys/class/hwmon")) {
            auto base = "/sys/class/hwmon/" + n;
            auto name = read_sysfs(base + "/name");
            for (auto& f : list_dir(base)) {
                if (starts_with(f, "temp") && ends_with(f, "_input")) {
                    auto v = to_f64(read_sysfs(base + "/" + f)).value_or(0) / 1000.0;
                    auto label = read_sysfs(base + "/" + f.substr(0, f.find('_')) + "_label");
                    std::ostringstream ss;
                    ss.setf(std::ios::fixed);
                    ss.precision(1);
                    ss << (label.empty() ? name : label) << ": " << v << " C";
                    temps.push_back(ss.str());
                }
                if (starts_with(f, "fan") && ends_with(f, "_input")) {
                    auto v = to_u64(read_sysfs(base + "/" + f)).value_or(0);
                    fans.push_back(name + ": " + std::to_string(v) + " RPM");
                }
                if (starts_with(f, "in") && ends_with(f, "_input") && o.extra >= 1) {
                    auto v = to_f64(read_sysfs(base + "/" + f)).value_or(0) / 1000.0;
                    std::ostringstream ss;
                    ss.setf(std::ios::fixed);
                    ss.precision(2);
                    ss << name << ": " << v << " V";
                    volts.push_back(ss.str());
                }
            }
        }
    }
    Row row;
    if (!temps.empty()) add_field(row, "System Temperatures", join(temps, " "));
    if (!fans.empty()) add_field(row, "Fan Speeds", join(fans, " "));
    if (!volts.empty()) add_field(row, "Voltages", join(volts, " "));
    if (row.fields.empty()) add_field(row, "Message", "No sensor data found");
    sec.rows.push_back(std::move(row));
    r.sections.push_back(std::move(sec));
}

void collect_slots(Report& r, const Options& o) {
    (void)o;
    Section sec;
    sec.title = "PCI Slots";
    Row row;
    if (which("dmidecode").empty()) {
        add_field(row, "Message", "dmidecode not available");
    } else {
        auto out = exec("dmidecode -t slot 2>/dev/null");
        if (trim(out).empty())
            add_field(row, "Message", "dmidecode requires root or no slot data");
        else {
            int n = 1;
            std::string type, des, use;
            auto flush = [&] {
                if (type.empty() && des.empty()) return;
                Row r2;
                add_field(r2, "Slot-" + std::to_string(n++), des.empty() ? type : des);
                if (!type.empty()) add_field(r2, "type", type);
                if (!use.empty()) add_field(r2, "status", use);
                sec.rows.push_back(std::move(r2));
                type.clear();
                des.clear();
                use.clear();
            };
            for (auto& line : split(out, '\n')) {
                auto t = trim(line);
                if (starts_with(t, "Handle")) flush();
                if (starts_with(t, "Type:")) type = trim(t.substr(5));
                if (starts_with(t, "Designation:")) des = trim(t.substr(12));
                if (starts_with(t, "Current Usage:")) use = trim(t.substr(14));
            }
            flush();
        }
    }
    if (sec.rows.empty()) sec.rows.push_back(std::move(row));
    r.sections.push_back(std::move(sec));
}

void collect_ram(Report& r, const Options& o) {
    Section sec;
    sec.title = "Memory";
    auto mem = mem_summary();
    Row sys;
    add_field(sys, "System RAM", "total: " + human_size(mem.total_kib));
    add_field(sys, "available", human_size(mem.available_kib));
    add_field(sys, "used", human_size(mem.used_kib) + " (" + percent(mem.used_kib, mem.total_kib) + ")");
    sec.rows.push_back(std::move(sys));

    if (!o.show_ram_short) {
        auto out = exec("dmidecode -t memory 2>/dev/null");
        if (trim(out).empty()) {
            Row row;
            add_field(row, "Message", "module data requires dmidecode (usually root)");
            sec.rows.push_back(row);
        } else {
            int n = 1;
            std::map<std::string, std::string> cur;
            auto flush = [&] {
                if (cur["Size"].empty() || cur["Size"] == "No Module Installed") {
                    if (!o.show_ram_modules && !cur.empty() && cur["Size"] == "No Module Installed") {
                        Row row;
                        add_field(row, "Device-" + std::to_string(n++), "empty");
                        if (!cur["Locator"].empty()) add_field(row, "locator", cur["Locator"]);
                        sec.rows.push_back(std::move(row));
                    }
                    cur.clear();
                    return;
                }
                Row row;
                add_field(row, "Device-" + std::to_string(n++), cur["Locator"].empty() ? "DIMM" : cur["Locator"]);
                add_field(row, "size", cur["Size"]);
                if (!cur["Type"].empty()) add_field(row, "type", cur["Type"]);
                if (!cur["Speed"].empty()) add_field(row, "speed", cur["Speed"]);
                if (o.extra >= 2 && !cur["Manufacturer"].empty())
                    add_field(row, "manufacturer", o.filter ? "<filter>" : cur["Manufacturer"]);
                if (o.extra >= 2 && !cur["Part Number"].empty())
                    add_field(row, "part-no", o.filter ? "<filter>" : cur["Part Number"]);
                sec.rows.push_back(std::move(row));
                cur.clear();
            };
            for (auto& line : split(out, '\n')) {
                auto t = trim(line);
                if (starts_with(t, "Memory Device")) flush();
                auto c = t.find(':');
                if (c != std::string::npos) cur[trim(t.substr(0, c))] = trim(t.substr(c + 1));
            }
            flush();
        }
    }
    r.sections.push_back(std::move(sec));
}

}  // namespace inxi
