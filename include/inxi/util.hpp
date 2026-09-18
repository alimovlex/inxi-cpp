#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace inxi {

std::string trim(std::string_view s);
std::string to_lower(std::string s);
std::vector<std::string> split(std::string_view s, char delim);
std::vector<std::string> split_ws(std::string_view s);
bool starts_with(std::string_view s, std::string_view p);
bool ends_with(std::string_view s, std::string_view p);
bool contains_ci(std::string_view hay, std::string_view needle);

std::string read_file(const std::string& path);
std::string read_sysfs(const std::string& path);
std::vector<std::string> read_lines(const std::string& path);
std::optional<std::string> try_read(const std::string& path);

bool exists(const std::string& path);
bool is_dir(const std::string& path);
std::vector<std::string> list_dir(const std::string& path);
std::string readlink_path(const std::string& path);
std::string basename_of(std::string_view path);
std::string which(const std::string& name);

std::string exec(const std::string& cmd);
std::optional<std::uint64_t> to_u64(std::string_view s);
std::optional<double> to_f64(std::string_view s);

std::string human_size(double kib, int digits = 2);
std::string human_size_bytes(std::uint64_t bytes, int digits = 2);
std::string percent(double num, double den, int digits = 1);
std::string format_uptime(double seconds);
std::string join(const std::vector<std::string>& v, std::string_view sep);

std::string clean_cpu_model(std::string model);
std::string core_count_words(unsigned n);
int term_width();
bool stdout_is_tty();
pid_t parent_pid(pid_t pid);
std::string proc_comm(pid_t pid);
std::string proc_cmdline(pid_t pid);
unsigned process_count();

std::unordered_map<std::string, std::string> parse_kv_file(const std::string& path,
                                                           char delim = '=');
std::string env(const char* key);

struct Mount {
    std::string device;
    std::string mountpoint;
    std::string fstype;
    std::string options;
};
std::vector<Mount> parse_mounts();

struct FsUsage {
    std::uint64_t total = 0;
    std::uint64_t used = 0;
    std::uint64_t free = 0;
};
std::optional<FsUsage> fs_usage(const std::string& path);

std::string filter_secret(std::string s, bool on);

}  // namespace inxi
