#include "inxi/collect.hpp"

#include "inxi/util.hpp"

#include <unistd.h>
#include <algorithm>
#include <cctype>
#include <map>
#include <sstream>

namespace inxi {
namespace {

std::string uname_field(char spec) {
    std::string cmd = "uname -";
    cmd.push_back(spec);
    return trim(exec(cmd));
}

std::string distro_name() {
    auto kv = parse_kv_file("/etc/os-release");
    if (auto it = kv.find("PRETTY_NAME"); it != kv.end() && !it->second.empty())
        return it->second;
    if (auto it = kv.find("NAME"); it != kv.end()) return it->second;
    if (exists("/etc/void-release") || exists("/etc/xbps.d")) return "Void Linux";
    if (exists("/etc/arch-release")) return "Arch Linux";
    if (exists("/etc/debian_version"))
        return "Debian " + read_sysfs("/etc/debian_version");
    return uname_field('s');
}

std::string desktop_info(const Options& o, std::string& version, std::string& toolkit,
                         std::string& tk_ver, std::string& wm) {
    auto xdg = to_lower(env("XDG_CURRENT_DESKTOP"));
    auto ds = to_lower(env("DESKTOP_SESSION"));
    std::string name;
    auto pick = [&](const std::string& s) {
        if (s.find("xfce") != std::string::npos) name = "Xfce";
        else if (s.find("kde") != std::string::npos || s.find("plasma") != std::string::npos)
            name = "KDE Plasma";
        else if (s.find("gnome") != std::string::npos) name = "GNOME";
        else if (s.find("mate") != std::string::npos) name = "MATE";
        else if (s.find("cinnamon") != std::string::npos) name = "Cinnamon";
        else if (s.find("lxqt") != std::string::npos) name = "LXQt";
        else if (s.find("lxde") != std::string::npos) name = "LXDE";
        else if (s.find("i3") != std::string::npos) name = "i3";
        else if (s.find("sway") != std::string::npos) name = "Sway";
        else if (s.find("hypr") != std::string::npos) name = "Hyprland";
        else if (s.find("budgie") != std::string::npos) name = "Budgie";
        else if (s.find("unity") != std::string::npos) name = "Unity";
        else if (!s.empty() && name.empty()) name = s;
    };
    pick(xdg);
    if (name.empty()) pick(ds);
    if (name.empty()) {
        if (!env("WAYLAND_DISPLAY").empty()) name = "Wayland";
        else if (!env("DISPLAY").empty()) name = "X11";
    }

    auto first_line_ver = [](const std::string& cmd, const std::string& needle = {}) {
        auto out = exec(cmd);
        for (auto& line : split(out, '\n')) {
            if (!needle.empty() && line.find(needle) == std::string::npos) continue;
            // grab last token that looks like a version
            for (auto& t : split_ws(line)) {
                if (!t.empty() && std::isdigit(static_cast<unsigned char>(t[0])) && t.find('.') != std::string::npos)
                    return t;
            }
        }
        return std::string{};
    };

    if (name == "Xfce") {
        version = first_line_ver("xfce4-session --version 2>/dev/null", "xfce4-session");
        if (version.empty()) version = first_line_ver("xfce4-about --version 2>/dev/null");
        toolkit = "Gtk";
        tk_ver = first_line_ver("pkg-config --modversion gtk+-3.0 2>/dev/null");
    } else if (name == "KDE Plasma") {
        version = env("KDE_SESSION_VERSION");
        if (auto p = which("plasmashell"); !p.empty())
            version = first_line_ver("plasmashell --version 2>/dev/null");
        toolkit = "Qt";
    } else if (name == "GNOME") {
        version = first_line_ver("gnome-shell --version 2>/dev/null");
        toolkit = "Gtk";
    }

    if (o.extra >= 2) {
        if (which("xprop").size() && !env("DISPLAY").empty()) {
            auto wp = exec("xprop -root _NET_SUPPORTING_WM_CHECK 2>/dev/null");
            (void)wp;
        }
        wm = env("XDG_CURRENT_DESKTOP");
    }
    return name.empty() ? "N/A" : name;
}

std::string init_system(std::string& version) {
    auto comm = proc_comm(1);
    if (comm == "systemd") {
        version = trim(exec("systemctl --version 2>/dev/null | head -n1"));
        auto sp = split_ws(version);
        if (sp.size() >= 2) version = sp[1];
        return "systemd";
    }
    if (comm == "runit" || exists("/etc/runit") || exists("/run/runit")) {
        return "runit";
    }
    if (comm == "openrc-init" || exists("/sbin/openrc")) return "OpenRC";
    if (comm == "init") {
        if (exists("/sbin/openrc")) return "OpenRC";
        return "SysVinit";
    }
    return comm.empty() ? "N/A" : comm;
}

std::string gcc_version() {
    auto p = which("gcc");
    if (p.empty()) return {};
    auto out = exec(p + " --version 2>/dev/null");
    auto line = split(out, '\n');
    if (line.empty()) return {};
    auto toks = split_ws(line[0]);
    if (!toks.empty()) return toks.back();
    return trim(line[0]);
}

std::string clang_version() {
    auto p = which("clang");
    if (p.empty()) return {};
    auto out = exec(p + " --version 2>/dev/null");
    auto toks = split_ws(split(out, '\n')[0]);
    for (size_t i = 0; i + 1 < toks.size(); ++i) {
        if (toks[i] == "version") return toks[i + 1];
    }
    return {};
}

std::string package_count() {
    std::vector<std::string> parts;
    if (which("xbps-query").size()) {
        auto n = trim(exec("xbps-query -l 2>/dev/null | wc -l"));
        if (!n.empty() && n != "0") parts.push_back("xbps: " + n);
    }
    if (which("dpkg-query").size()) {
        auto n = trim(exec("dpkg-query -f '${binary:Package}\\n' -W 2>/dev/null | wc -l"));
        if (!n.empty() && n != "0") parts.push_back("dpkg: " + n);
    }
    if (which("rpm").size()) {
        auto n = trim(exec("rpm -qa 2>/dev/null | wc -l"));
        if (!n.empty() && n != "0") parts.push_back("rpm: " + n);
    }
    if (which("pacman").size()) {
        auto n = trim(exec("pacman -Qq 2>/dev/null | wc -l"));
        if (!n.empty() && n != "0") parts.push_back("pacman: " + n);
    }
    if (which("flatpak").size()) {
        auto n = trim(exec("flatpak list --app 2>/dev/null | wc -l"));
        if (!n.empty() && n != "0") parts.push_back("flatpak: " + n);
    }
    return join(parts, " ");
}

std::string parent_shell(std::string& version) {
    pid_t p = parent_pid(getpid());
    std::string name;
    for (int i = 0; i < 8 && p > 1; ++i) {
        name = proc_comm(p);
        static const char* skip[] = {"inxi", "timeout", "stdbuf", "script", "strace"};
        bool sk = false;
        for (auto s : skip)
            if (name == s) sk = true;
        if (!sk && !name.empty()) break;
        p = parent_pid(p);
    }
    if (name.empty()) name = env("SHELL");
    name = basename_of(name);
    if (name == "bash" || name == "zsh" || name == "fish" || name == "ksh" || name == "dash") {
        auto out = exec(name + " --version 2>/dev/null");
        auto toks = split_ws(split(out, '\n')[0]);
        for (auto& t : toks) {
            if (!t.empty() && std::isdigit(static_cast<unsigned char>(t[0]))) {
                version = t;
                break;
            }
        }
    }
    return name.empty() ? "N/A" : name;
}

}  // namespace

void collect_system(Report& r, const Options& o) {
    Section sec;
    sec.title = "System";
    Row krow;
    auto rel = uname_field('r');
    auto arch = uname_field('m');
    add_field(krow, "Kernel", rel);
    add_field(krow, "arch", arch);
    add_field(krow, "bits", (arch.find("64") != std::string::npos) ? "64" : "32");
    if (o.extra >= 1) {
        auto vmlinuz = read_sysfs("/proc/version");
        auto gccpos = vmlinuz.find("gcc version");
        if (gccpos != std::string::npos) {
            auto rest = split_ws(vmlinuz.substr(gccpos));
            if (rest.size() >= 3) add_field(krow, "compiler", "gcc " + rest[2]);
        }
    }
    sec.rows.push_back(std::move(krow));

    Row drow;
    std::string ver, toolkit, tk_ver, wm;
    auto desk = desktop_info(o, ver, toolkit, tk_ver, wm);
    add_field(drow, "Desktop", desk);
    if (!ver.empty()) add_field(drow, "v", ver);
    if (o.extra >= 2 && !toolkit.empty()) {
        add_field(drow, "tk", toolkit);
        if (!tk_ver.empty()) add_field(drow, "v", tk_ver);
    }
    if (o.extra >= 2) {
        auto dm = env("XDG_SESSION_DESKTOP");
        (void)dm;
        if (exists("/run/lightdm")) add_field(drow, "dm", "lightdm");
        else if (exists("/run/gdm") || exists("/run/gdm3")) add_field(drow, "dm", "gdm");
        else if (exists("/run/sddm")) add_field(drow, "dm", "sddm");
        else if (exists("/run/lxdm")) add_field(drow, "dm", "lxdm");
    }
    add_field(drow, "Distro", distro_name());
    sec.rows.push_back(std::move(drow));
    r.sections.push_back(std::move(sec));
}

MemShort mem_summary() {
    MemShort m;
    auto kv = parse_kv_file("/proc/meminfo", ':');
    auto kib = [](const std::string& s) {
        auto t = split_ws(s);
        return t.empty() ? 0.0 : to_f64(t[0]).value_or(0);
    };
    m.total_kib = kib(kv["MemTotal"]);
    m.available_kib = kib(kv["MemAvailable"]);
    if (m.available_kib <= 0) m.available_kib = kib(kv["MemFree"]);
    m.used_kib = m.total_kib > m.available_kib ? m.total_kib - m.available_kib : 0;
    return m;
}

void collect_info(Report& r, const Options& o) {
    Section sec;
    sec.title = "Info";
    auto mem = mem_summary();
    Row mrow;
    add_field(mrow, "Memory", "total: " + human_size(mem.total_kib));
    add_field(mrow, "available", human_size(mem.available_kib));
    add_field(mrow, "used", human_size(mem.used_kib) + " (" +
                                percent(mem.used_kib, mem.total_kib) + ")");
    sec.rows.push_back(std::move(mrow));

    Row prow;
    add_field(prow, "Processes", std::to_string(process_count()));
    auto up = to_f64(split_ws(read_file("/proc/uptime")).empty()
                         ? "0"
                         : split_ws(read_file("/proc/uptime"))[0]);
    add_field(prow, "Uptime", format_uptime(up.value_or(0)));

    if (o.extra >= 1 || true) {
        std::string iv;
        auto init = init_system(iv);
        if (o.extra >= 1) {
            add_field(prow, "Init", init);
            if (o.extra >= 2 && !iv.empty()) add_field(prow, "v", iv);
        }
    }
    if (o.extra >= 1) {
        auto gcc = gcc_version();
        auto cl = clang_version();
        if (!gcc.empty() || !cl.empty()) {
            add_field(prow, "Compilers", "");
            if (!gcc.empty()) add_field(prow, "gcc", gcc);
            if (!cl.empty()) add_field(prow, "clang", cl);
        }
        auto pkgs = package_count();
        if (!pkgs.empty()) add_field(prow, "Packages", pkgs);
    }
    std::string shver;
    auto sh = parent_shell(shver);
    add_field(prow, "Shell", sh);
    if (o.extra >= 1 && !shver.empty()) add_field(prow, "v", shver);
    add_field(prow, "inxi", INXI_CPP_VERSION);
    sec.rows.push_back(std::move(prow));
    r.sections.push_back(std::move(sec));
}

void collect_processes(Report& r, const Options& o) {
    struct Proc {
        pid_t pid = 0;
        std::string comm;
        double pcpu = 0;
        double rss_kib = 0;
    };
    std::vector<Proc> ps;
    for (auto& n : list_dir("/proc")) {
        if (n.empty() || !std::isdigit(static_cast<unsigned char>(n[0]))) continue;
        Proc p;
        p.pid = static_cast<pid_t>(to_u64(n).value_or(0));
        p.comm = proc_comm(p.pid);
        auto st = read_file("/proc/" + n + "/stat");
        auto rp = st.rfind(')');
        if (rp != std::string::npos) {
            auto rest = split_ws(st.substr(rp + 1));
            // fields: 3=state ... 14=utime 15=stime (1-based from comm)
            if (rest.size() > 13) {
                double ut = to_f64(rest[11]).value_or(0);
                double stt = to_f64(rest[12]).value_or(0);
                p.pcpu = ut + stt;
            }
        }
        auto status = parse_kv_file("/proc/" + n + "/status", ':');
        auto vm = split_ws(status["VmRSS"]);
        if (!vm.empty()) p.rss_kib = to_f64(vm[0]).value_or(0);
        if (!p.comm.empty()) ps.push_back(p);
    }

    Section sec;
    sec.title = "Processes";
    int n = o.ps_count > 0 ? o.ps_count : 5;
    if (o.show_ps_cpu) {
        auto cpu = ps;
        std::sort(cpu.begin(), cpu.end(), [](auto& a, auto& b) { return a.pcpu > b.pcpu; });
        Row row;
        add_field(row, "CPU top", std::to_string(n));
        sec.rows.push_back(row);
        for (int i = 0; i < n && i < static_cast<int>(cpu.size()); ++i) {
            Row r2;
            add_field(r2, std::to_string(i + 1), cpu[i].comm);
            add_field(r2, "pid", std::to_string(cpu[i].pid));
            add_field(r2, "cpu", std::to_string(static_cast<int>(cpu[i].pcpu)));
            sec.rows.push_back(std::move(r2));
        }
    }
    if (o.show_ps_mem) {
        auto mem = ps;
        std::sort(mem.begin(), mem.end(), [](auto& a, auto& b) { return a.rss_kib > b.rss_kib; });
        Row row;
        add_field(row, "Memory top", std::to_string(n));
        sec.rows.push_back(row);
        for (int i = 0; i < n && i < static_cast<int>(mem.size()); ++i) {
            Row r2;
            add_field(r2, std::to_string(i + 1), mem[i].comm);
            add_field(r2, "pid", std::to_string(mem[i].pid));
            add_field(r2, "rss", human_size(mem[i].rss_kib));
            sec.rows.push_back(std::move(r2));
        }
    }
    r.sections.push_back(std::move(sec));
}

void collect_repos(Report& r, const Options& o) {
    (void)o;
    Section sec;
    sec.title = "Repos";
    auto add_files = [&](const std::string& title, const std::string& dir, const std::string& suffix) {
        if (!is_dir(dir)) return;
        for (auto& n : list_dir(dir)) {
            if (!suffix.empty() && !ends_with(n, suffix)) continue;
            Row row;
            add_field(row, title, n);
            auto body = trim(read_file(dir + "/" + n));
            if (!body.empty()) {
                auto first = split(body, '\n')[0];
                if (!starts_with(trim(first), "#")) add_field(row, "file", trim(first));
            }
            sec.rows.push_back(std::move(row));
        }
    };
    add_files("Active xbps", "/etc/xbps.d", ".conf");
    add_files("xbps defaults", "/usr/share/xbps.d", ".conf");
    if (exists("/etc/pacman.conf")) {
        Row row;
        add_field(row, "Active pacman", "/etc/pacman.conf");
        sec.rows.push_back(row);
    }
    if (is_dir("/etc/apt")) {
        add_files("Active apt", "/etc/apt/sources.list.d", ".list");
        if (exists("/etc/apt/sources.list")) {
            Row row;
            add_field(row, "Active apt", "/etc/apt/sources.list");
            sec.rows.push_back(row);
        }
    }
    if (exists("/etc/yum.repos.d") || is_dir("/etc/yum.repos.d"))
        add_files("Active yum", "/etc/yum.repos.d", ".repo");
    if (is_dir("/etc/zypp/repos.d")) add_files("Active zypp", "/etc/zypp/repos.d", ".repo");
    if (sec.rows.empty()) {
        Row row;
        add_field(row, "No repo files", "detected");
        sec.rows.push_back(row);
    }
    r.sections.push_back(std::move(sec));
}

void collect_weather(Report& r, const Options& o) {
    Section sec;
    sec.title = "Weather";
    Row row;
    auto curl = which("curl");
    auto wget = which("wget");
    if (curl.empty() && wget.empty()) {
        add_field(row, "Report", "no downloader (install curl)");
        sec.rows.push_back(row);
        r.sections.push_back(std::move(sec));
        return;
    }
    std::string loc = o.weather_location.empty() ? "" : o.weather_location;
    std::string url = "https://wttr.in/" + loc + "?format=3";
    std::string cmd = !curl.empty() ? curl + " -fsS --max-time 4 " + url + " 2>/dev/null"
                                    : wget + " -q -T 4 -O - " + url + " 2>/dev/null";
    auto out = trim(exec(cmd));
    add_field(row, "Report", out.empty() ? "unavailable" : out);
    sec.rows.push_back(row);
    r.sections.push_back(std::move(sec));
}

}  // namespace inxi
