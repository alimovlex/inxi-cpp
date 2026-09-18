#include "inxi/util.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <climits>

namespace inxi {

std::string trim(std::string_view s) {
    auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string_view::npos) return {};
    auto e = s.find_last_not_of(" \t\r\n");
    return std::string{s.substr(b, e - b + 1)};
}

std::string to_lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::vector<std::string> split(std::string_view s, char delim) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == delim) {
            out.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    out.push_back(cur);
    return out;
}

std::vector<std::string> split_ws(std::string_view s) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!cur.empty()) {
                out.push_back(cur);
                cur.clear();
            }
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

bool starts_with(std::string_view s, std::string_view p) {
    return s.size() >= p.size() && s.substr(0, p.size()) == p;
}

bool ends_with(std::string_view s, std::string_view p) {
    return s.size() >= p.size() && s.substr(s.size() - p.size()) == p;
}

bool contains_ci(std::string_view hay, std::string_view needle) {
    auto h = to_lower(std::string{hay});
    auto n = to_lower(std::string{needle});
    return h.find(n) != std::string::npos;
}

std::string read_file(const std::string& path) {
    std::ifstream in(path);
    if (!in) return {};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::string read_sysfs(const std::string& path) {
    return trim(read_file(path));
}

std::optional<std::string> try_read(const std::string& path) {
    std::ifstream in(path);
    if (!in) return std::nullopt;
    std::ostringstream ss;
    ss << in.rdbuf();
    return trim(ss.str());
}

std::vector<std::string> read_lines(const std::string& path) {
    std::vector<std::string> lines;
    std::ifstream in(path);
    if (!in) return lines;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
    }
    return lines;
}

bool exists(const std::string& path) {
    struct stat st{};
    return ::stat(path.c_str(), &st) == 0;
}

bool is_dir(const std::string& path) {
    struct stat st{};
    if (::stat(path.c_str(), &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}

std::vector<std::string> list_dir(const std::string& path) {
    std::vector<std::string> names;
    DIR* d = opendir(path.c_str());
    if (!d) return names;
    while (auto* ent = readdir(d)) {
        std::string n = ent->d_name;
        if (n == "." || n == "..") continue;
        names.push_back(n);
    }
    closedir(d);
    std::sort(names.begin(), names.end());
    return names;
}

std::string readlink_path(const std::string& path) {
    char buf[PATH_MAX];
    ssize_t n = ::readlink(path.c_str(), buf, sizeof(buf) - 1);
    if (n < 0) return {};
    buf[n] = 0;
    return buf;
}

std::string basename_of(std::string_view path) {
    auto p = path.find_last_of('/');
    if (p == std::string_view::npos) return std::string{path};
    return std::string{path.substr(p + 1)};
}

std::string which(const std::string& name) {
    if (name.find('/') != std::string::npos) {
        return exists(name) ? name : "";
    }
    const char* path = std::getenv("PATH");
    if (!path) path = "/usr/bin:/bin:/usr/sbin:/sbin";
    for (auto& dir : split(path, ':')) {
        if (dir.empty()) continue;
        auto p = dir + "/" + name;
        if (access(p.c_str(), X_OK) == 0) return p;
    }
    return {};
}

std::string exec(const std::string& cmd) {
    FILE* p = popen(cmd.c_str(), "r");
    if (!p) return {};
    std::string out;
    char buf[4096];
    while (fgets(buf, sizeof(buf), p)) out += buf;
    pclose(p);
    return out;
}

std::optional<std::uint64_t> to_u64(std::string_view s) {
    if (s.empty()) return std::nullopt;
    try {
        size_t idx = 0;
        auto v = std::stoull(std::string{s}, &idx, 0);
        if (idx == 0) return std::nullopt;
        return v;
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<double> to_f64(std::string_view s) {
    if (s.empty()) return std::nullopt;
    try {
        size_t idx = 0;
        auto v = std::stod(std::string{s}, &idx);
        if (idx == 0) return std::nullopt;
        return v;
    } catch (...) {
        return std::nullopt;
    }
}

static std::string fmt_num(double v, int digits) {
    std::ostringstream ss;
    ss.setf(std::ios::fixed);
    ss.precision(digits);
    ss << v;
    auto s = ss.str();
    if (s.find('.') != std::string::npos) {
        while (!s.empty() && s.back() == '0') s.pop_back();
        if (!s.empty() && s.back() == '.') s.pop_back();
    }
    return s;
}

std::string human_size_bytes(std::uint64_t bytes, int digits) {
    static const char* units[] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB"};
    double v = static_cast<double>(bytes);
    int u = 0;
    while (v >= 1024.0 && u < 5) {
        v /= 1024.0;
        ++u;
    }
    if (u == 0) digits = 0;
    return fmt_num(v, digits) + " " + units[u];
}

std::string human_size(double kib, int digits) {
    return human_size_bytes(static_cast<std::uint64_t>(std::llround(kib * 1024.0)), digits);
}

std::string percent(double num, double den, int digits) {
    if (den <= 0) return "0%";
    std::ostringstream ss;
    ss.setf(std::ios::fixed);
    ss.precision(digits);
    ss << (num / den * 100.0) << "%";
    return ss.str();
}

std::string format_uptime(double seconds) {
    auto s = static_cast<long>(seconds);
    long d = s / 86400;
    s %= 86400;
    long h = s / 3600;
    s %= 3600;
    long m = s / 60;
    std::string out;
    if (d) out += std::to_string(d) + "d ";
    if (d || h) out += std::to_string(h) + "h ";
    out += std::to_string(m) + "m";
    return out;
}

std::string join(const std::vector<std::string>& v, std::string_view sep) {
    std::string o;
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) o += sep;
        o += v[i];
    }
    return o;
}

std::string clean_cpu_model(std::string model) {
    auto strip = [&](const std::string& what) {
        for (;;) {
            auto p = model.find(what);
            if (p == std::string::npos) break;
            model.erase(p, what.size());
        }
    };
    strip("(R)");
    strip("(TM)");
    strip("(tm)");
    strip("(r)");
    strip(" CPU");
    auto at = model.find(" @");
    if (at != std::string::npos) model.erase(at);
    at = model.find(" Processor");
    // keep Processor for AMD sometimes; inxi strips Intel CPU @ freq already
    std::string out;
    bool sp = false;
    for (char c : model) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!sp && !out.empty()) {
                out.push_back(' ');
                sp = true;
            }
        } else {
            out.push_back(c);
            sp = false;
        }
    }
    return trim(out);
}

std::string core_count_words(unsigned n) {
    switch (n) {
        case 1: return "single core";
        case 2: return "dual core";
        case 3: return "triple core";
        case 4: return "quad core";
        case 6: return "6-core";
        case 8: return "8-core";
        case 12: return "12-core";
        case 16: return "16-core";
        default: return std::to_string(n) + "-core";
    }
}

int term_width() {
    winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
        return ws.ws_col;
    if (const char* c = std::getenv("COLUMNS")) {
        if (auto n = to_u64(c); n && *n > 0) return static_cast<int>(*n);
    }
    return 80;
}

bool stdout_is_tty() { return isatty(STDOUT_FILENO) == 1; }

pid_t parent_pid(pid_t pid) {
    auto txt = read_file("/proc/" + std::to_string(pid) + "/stat");
    auto rparen = txt.rfind(')');
    if (rparen == std::string::npos) return 0;
    auto rest = split_ws(txt.substr(rparen + 1));
    if (rest.size() < 2) return 0;
    return static_cast<pid_t>(to_u64(rest[1]).value_or(0));
}

std::string proc_comm(pid_t pid) {
    auto s = read_sysfs("/proc/" + std::to_string(pid) + "/comm");
    return s;
}

std::string proc_cmdline(pid_t pid) {
    auto s = read_file("/proc/" + std::to_string(pid) + "/cmdline");
    for (char& c : s) if (c == '\0') c = ' ';
    return trim(s);
}

unsigned process_count() {
    unsigned n = 0;
    for (auto& name : list_dir("/proc")) {
        if (!name.empty() && std::isdigit(static_cast<unsigned char>(name[0]))) ++n;
    }
    return n;
}

std::unordered_map<std::string, std::string> parse_kv_file(const std::string& path, char delim) {
    std::unordered_map<std::string, std::string> m;
    for (auto& line : read_lines(path)) {
        auto t = trim(line);
        if (t.empty() || t[0] == '#') continue;
        auto p = t.find(delim);
        if (p == std::string::npos) continue;
        auto k = trim(t.substr(0, p));
        auto v = trim(t.substr(p + 1));
        if (!v.empty() && v.front() == '"' && v.back() == '"')
            v = v.substr(1, v.size() - 2);
        m[k] = v;
    }
    return m;
}

std::string env(const char* key) {
    const char* v = std::getenv(key);
    return v ? std::string{v} : std::string{};
}

std::vector<Mount> parse_mounts() {
    std::vector<Mount> out;
    for (auto& line : read_lines("/proc/mounts")) {
        auto p = split_ws(line);
        if (p.size() < 4) continue;
        Mount m;
        m.device = p[0];
        m.mountpoint = p[1];
        // unescape octal in mountpoint
        std::string mp;
        for (size_t i = 0; i < m.mountpoint.size(); ++i) {
            if (m.mountpoint[i] == '\\' && i + 3 < m.mountpoint.size()) {
                int v = (m.mountpoint[i + 1] - '0') * 64 + (m.mountpoint[i + 2] - '0') * 8 +
                        (m.mountpoint[i + 3] - '0');
                mp.push_back(static_cast<char>(v));
                i += 3;
            } else {
                mp.push_back(m.mountpoint[i]);
            }
        }
        m.mountpoint = mp;
        m.fstype = p[2];
        m.options = p[3];
        out.push_back(std::move(m));
    }
    return out;
}

std::optional<FsUsage> fs_usage(const std::string& path) {
    struct statvfs st{};
    if (statvfs(path.c_str(), &st) != 0) return std::nullopt;
    FsUsage u;
    u.total = static_cast<std::uint64_t>(st.f_blocks) * st.f_frsize;
    u.free = static_cast<std::uint64_t>(st.f_bavail) * st.f_frsize;
    u.used = u.total > u.free ? u.total - u.free : 0;
    return u;
}

std::string filter_secret(std::string s, bool on) {
    if (!on) return s;
    if (const char* user = std::getenv("USER"); user && *user) {
        std::string u = user;
        std::string home = "/home/" + u;
        auto pos = s.find(home);
        while (pos != std::string::npos) {
            s.replace(pos, home.size(), "/home/<filter>");
            pos = s.find(home, pos + 14);
        }
    }
    return s;
}

}  // namespace inxi
