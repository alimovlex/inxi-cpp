// system_item.h
// Collects OS / kernel / desktop environment / shell info.
// Perl: SystemItem, system_data() helpers
//
// Perl subs mapped:
//   get()              → SystemItem::get()
//   full_output()      → SystemItem::full_output()
//   short_output()     → SystemItem::short_output()
//   system_data()      → SystemItem::system_data()
//   kernel_data()      → SystemItem::kernel_data()
//   distro_data()      → SystemItem::distro_data()
//   de_data()          → SystemItem::de_data()       [desktop env]
//   shell_data()       → SystemItem::shell_data()
//   wm_data()          → SystemItem::wm_data()       [window manager]
//   theme_data()       → SystemItem::theme_data()
//   uptime_data()      → SystemItem::uptime_data()
#pragma once
#include "items/info_item.h"
#include <string>
#include <vector>

namespace inxi {

struct SystemData {
    std::string hostname;
    std::string kernel;       // "6.1.0-27-amd64"
    std::string kernel_bits;  // "64-bit"
    std::string distro;       // "Debian GNU/Linux 12"
    std::string distro_id;    // "debian"
    std::string distro_ver;   // "12"
    std::string desktop;      // "KDE Plasma 5.27"
    std::string de_id;
    std::string wm;           // "KWin"|"Openbox"
    std::string wm_theme;
    std::string dm;           // "SDDM"|"GDM"
    std::string icon_theme;
    std::string cursor_theme;
    std::string shell;        // "bash 5.2.15"
    std::string shell_id;     // "bash"
    std::string shell_ver;
    std::string uptime;       // "1d 3h 42m"
    std::string packages;     // "1234 (dpkg), 512 (flatpak)"
    std::string running_in;   // "SSH"|"Console"|""
    std::string locale;
    std::string boot_id;
    std::string init;         // "systemd"|"openrc"|"runit"
    std::string init_ver;
    bool        is_live = false;
};

class SystemItem final : public InfoItem {
public:
    explicit SystemItem(GlobalState& s) : InfoItem(s) {}
    std::string label() const override { return "System"; }
    std::vector<OutputRow> get() override;

private:
    std::vector<OutputRow> full_output(const SystemData& d);
    std::vector<OutputRow> short_output(const SystemData& d);

    SystemData  system_data();
    void        kernel_data(SystemData& d);
    void        distro_data(SystemData& d);
    void        de_data(SystemData& d);
    void        shell_data(SystemData& d);
    void        wm_data(SystemData& d);
    void        theme_data(SystemData& d);
    std::string uptime_data();
};

}  // namespace inxi
