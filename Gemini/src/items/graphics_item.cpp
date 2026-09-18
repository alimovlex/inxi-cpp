// graphics_item.cpp
#include "items/graphics_item.h"
#include "utils/file_utils.h"
#include "utils/process_utils.h"
#include "utils/string_utils.h"
#include <cstdlib>
#include <filesystem>
#include <regex>

namespace inxi {

std::string GraphicsItem::driver_data(const GpuDevice& gpu) {
    return gpu.driver;
}

std::vector<GpuDevice> GraphicsItem::gpu_data() {
    std::vector<GpuDevice> gpus;
    // Check PCI display devices via lspci or sysfs
    auto lines = utils::run_capture("lspci -nnk");
    GpuDevice cur_gpu;
    bool in_display = false;
    int dev_id = 1;

    for (const auto& line : lines) {
        if (line.find("VGA compatible controller") != std::string::npos ||
            line.find("3D controller") != std::string::npos ||
            line.find("Display controller") != std::string::npos) {
            if (in_display && !cur_gpu.model.empty()) {
                gpus.push_back(cur_gpu);
            }
            in_display = true;
            cur_gpu = GpuDevice();
            cur_gpu.id = "Device-" + std::to_string(dev_id++);
            // Parse model from line
            size_t colon = line.find(": ");
            if (colon != std::string::npos) {
                std::string rest = line.substr(colon + 2);
                size_t sub_colon = rest.find(": ");
                if (sub_colon != std::string::npos) {
                    cur_gpu.model = utils::clean(utils::clean_pci(rest.substr(sub_colon + 2)));
                } else {
                    cur_gpu.model = utils::clean(utils::clean_pci(rest));
                }
            }
        } else if (in_display) {
            if (line.find("Kernel driver in use:") != std::string::npos) {
                cur_gpu.driver = utils::trim(line.substr(line.find(':') + 1));
            } else if (!line.empty() && line[0] != '\t' && line[0] != ' ') {
                gpus.push_back(cur_gpu);
                in_display = false;
                cur_gpu = GpuDevice();
            }
        }
    }
    if (in_display && !cur_gpu.model.empty()) {
        gpus.push_back(cur_gpu);
    }

    // Also check USB webcams / video devices if any
    auto uvc_lines = utils::run_capture("lsusb");
    for (const auto& line : uvc_lines) {
        if (line.find("Camera") != std::string::npos || line.find("Webcam") != std::string::npos) {
            GpuDevice usb_cam;
            usb_cam.id = "Device-" + std::to_string(dev_id++);
            size_t id_pos = line.find("ID ");
            if (id_pos != std::string::npos && line.size() > id_pos + 13) {
                usb_cam.model = utils::clean(utils::trim(line.substr(id_pos + 13)));
            } else {
                usb_cam.model = "Webcam";
            }
            usb_cam.driver = "uvcvideo";
            gpus.push_back(usb_cam);
            break;
        }
    }

    return gpus;
}

DisplayServer GraphicsItem::display_data() {
    DisplayServer disp;
    const char* wayland = std::getenv("WAYLAND_DISPLAY");
    const char* display = std::getenv("DISPLAY");

    if (wayland && *wayland) {
        disp.type = "wayland";
        wayland_data(disp);
    } else if (display && *display) {
        disp.type = "x11";
        xorg_data(disp);
    }

    // OpenGL API
    auto gl_lines = utils::run_capture("glxinfo -B");
    for (const auto& l : gl_lines) {
        if (l.find("OpenGL version string:") != std::string::npos) {
            disp.version_gl = utils::trim(l.substr(l.find(':') + 1));
        } else if (l.find("OpenGL vendor string:") != std::string::npos) {
            disp.vendor_gl = utils::trim(l.substr(l.find(':') + 1));
        } else if (l.find("OpenGL renderer string:") != std::string::npos) {
            disp.renderer = utils::trim(l.substr(l.find(':') + 1));
        }
    }

    return disp;
}

void GraphicsItem::xorg_data(DisplayServer& disp) {
    auto x_ver = utils::run_capture("Xorg -version");
    for (const auto& l : x_ver) {
        if (l.find("X.Org X Server") != std::string::npos) {
            disp.version = utils::trim(l.substr(l.find("X Server") + 8));
            break;
        }
    }
}

void GraphicsItem::wayland_data(DisplayServer& disp) {}
std::vector<MonitorInfo> GraphicsItem::monitor_data() { return {}; }
DisplayServer GraphicsItem::compositor_data() { return display_data(); }

std::vector<OutputRow> GraphicsItem::gpu_output(const std::vector<GpuDevice>& gpus) {
    std::vector<OutputRow> rows;
    bool is_first = true;
    for (const auto& g : gpus) {
        std::vector<Field> fields;
        fields.push_back(field(g.id, g.model));
        if (!g.driver.empty()) {
            fields.push_back(field("driver", g.driver));
            if (g.driver != "uvcvideo") {
                fields.push_back(field("v", "kernel"));
            } else {
                fields.push_back(field("type", "USB"));
            }
        }
        if (is_first) {
            rows.push_back(make_row("Graphics", fields));
            is_first = false;
        } else {
            rows.push_back(make_row("", fields, true));
        }
    }
    return rows;
}

std::vector<OutputRow> GraphicsItem::display_output(const DisplayServer& disp) {
    std::vector<OutputRow> rows;
    if (!disp.type.empty()) {
        std::vector<Field> f;
        f.push_back(field("Display", disp.type));
        if (!disp.version.empty()) {
            f.push_back(field("server", "X.Org " + disp.version));
        }
        // Resolution from sysfs
        auto mode_files = utils::glob_files("/sys/class/drm/card*-*/modes");
        std::string res;
        for (const auto& mf : mode_files) {
            std::string m = utils::read_file_str(mf);
            if (!m.empty()) {
                res = utils::trim(utils::split_lines(m)[0]);
                break;
            }
        }
        if (!res.empty()) {
            f.push_back(field("resolution", res));
        }
        rows.push_back(make_row("", f, true));
    }

    if (!disp.renderer.empty()) {
        std::vector<Field> f_gl;
        f_gl.push_back(field("API", "OpenGL"));
        if (!disp.version_gl.empty()) f_gl.push_back(field("v", disp.version_gl));
        if (!disp.renderer.empty()) f_gl.push_back(field("renderer", disp.renderer));
        rows.push_back(make_row("", f_gl, true));
    }

    return rows;
}

std::vector<OutputRow> GraphicsItem::monitor_output(const std::vector<MonitorInfo>& monitors) { return {}; }
std::vector<OutputRow> GraphicsItem::compositor_output(const DisplayServer& disp) { return {}; }

std::vector<OutputRow> GraphicsItem::get() {
    auto gpus = gpu_data();
    auto disp = display_data();
    auto rows = gpu_output(gpus);
    auto d_rows = display_output(disp);
    rows.insert(rows.end(), d_rows.begin(), d_rows.end());
    return rows;
}

}  // namespace inxi
