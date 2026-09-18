// file_utils.h
// File / filesystem / glob utilities.
// Perl subs mapped:
//   reader()        → read_file()
//   writer()        → write_file()
//   toucher()       → touch_file()
//   globber()       → glob_files()
//   count_dir_files()→ count_dir_files()
//   check_output_path() → check_output_path()
//   load_json()     → load_json()     [minimal JSON reader for config]
#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace inxi::utils {


// Read all lines of a file (returns empty vec if unreadable)
std::vector<std::string> read_file(const std::string& path);

// Read entire file as one string
std::string read_file_str(const std::string& path);

// Write lines to a file (creates/overwrites)
bool write_file(const std::string& path, const std::vector<std::string>& lines);
bool write_file(const std::string& path, const std::string& content);

// Create/update file mtime (like POSIX touch)
bool touch_file(const std::string& path);

// Glob expansion  — e.g. glob_files("/sys/class/hwmon/hwmon*/name")
std::vector<std::string> glob_files(const std::string& pattern);

// Count files in directory
int count_dir_files(const std::string& dir, bool recursive = false);

// Validate / create output file path
bool check_output_path(const std::string& path);

// Minimal flat key=value reader (for inxi config files)
// Returns map of key→value
std::unordered_map<std::string, std::string>
    load_config(const std::string& path);

}  // namespace inxi::utils
