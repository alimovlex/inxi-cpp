// graphics_item.h
// Collects GPU / display-server / compositor information.
// Perl: DisplayItem / GraphicsItem subs
//
// Perl subs mapped:
//   get()                    → GraphicsItem::get()
//   gpu_output()             → GraphicsItem::gpu_output()
//   display_output()         → GraphicsItem::display_output()
//   monitor_output()         → GraphicsItem::monitor_output()
//   compositor_output()      → GraphicsItem::compositor_output()
//   gpu_data()               → GraphicsItem::gpu_data()
//   display_data()           → GraphicsItem::display_data()
//   monitor_data()           → GraphicsItem::monitor_data()
//   compositor_data()        → GraphicsItem::compositor_data()
//   xorg_data()              → GraphicsItem::xorg_data()
//   wayland_data()           → GraphicsItem::wayland_data()
//   driver_data()            → GraphicsItem::driver_data()
#pragma once
#include "items/info_item.h"
#include <string>
#include <vector>

namespace inxi {

struct GpuDevice {
    std::string id;
    std::string vendor;
    std::string model;
    std::string driver;
    std::string driver_version;
    std::string bus_id;
    std::string memory;
    std::string temp;
    std::string perf_state;
    bool        is_primary = false;
};

struct MonitorInfo {
    std::string name;
    std::string vendor;
    std::string model;
    std::string serial;
    std::string resolution;   // e.g. "1920x1080"
    std::string refresh_rate; // e.g. "60Hz"
    std::string size_cm;      // physical size
    std::string depth;        // color depth
    bool        built_in = false;
};

struct DisplayServer {
    std::string type;       // "X11"|"Wayland"|"Mir"|""
    std::string compositor; // e.g. "KWin"|"Mutter"|"Sway"
    std::string version;
    std::string renderer;
    std::string vendor_gl;
    std::string version_gl;
    std::string version_glsl;
};

class GraphicsItem final : public InfoItem {
public:
    explicit GraphicsItem(GlobalState& s) : InfoItem(s) {}
    std::string label() const override { return "Graphics"; }
    std::vector<OutputRow> get() override;

private:
    std::vector<OutputRow> gpu_output(const std::vector<GpuDevice>& gpus);
    std::vector<OutputRow> display_output(const DisplayServer& disp);
    std::vector<OutputRow> monitor_output(const std::vector<MonitorInfo>& monitors);
    std::vector<OutputRow> compositor_output(const DisplayServer& disp);

    std::vector<GpuDevice>   gpu_data();
    DisplayServer            display_data();
    std::vector<MonitorInfo> monitor_data();
    DisplayServer            compositor_data();
    void                     xorg_data(DisplayServer& disp);
    void                     wayland_data(DisplayServer& disp);
    std::string              driver_data(const GpuDevice& gpu);
};

}  // namespace inxi
