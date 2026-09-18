// process_utils.h
// Process execution and program-lookup utilities.
// Perl subs mapped:
//   check_program()      → check_program()
//   grabber()            → run_capture()
//   program_data()       → program_data()
//   program_version()    → program_version()
//   program_version_cmd()→ program_version_cmd()
//   program_version_pkg()→ program_version_pkg()
//   awk()                → awk_field()
//   get_pms()            → get_package_managers()
//   get_defined()        → get_defined()
#pragma once
#include <string>
#include <vector>

namespace inxi::utils {

// Perl: check_program() — searches PATH for executable; returns full path or ""
std::string check_program(const std::string& name,
                           const std::vector<std::string>& paths = {});

// Perl: grabber("cmd arg…") — run command, capture stdout as lines
// stderr is discarded (like 2>/dev/null)
std::vector<std::string> run_capture(const std::string& cmd);

// Perl: grabber with stderr merged (2>&1 variant)
std::vector<std::string> run_capture_merged(const std::string& cmd);

// Perl: program_data("prog") → [name, version, path]
// Returns {name, version, path}
struct ProgramInfo {
    std::string name;
    std::string version;
    std::string path;
};
ProgramInfo program_data(const std::string& prog,
                          const std::vector<std::string>& paths = {});

// Perl: program_version() — detect version from various heuristics
std::string program_version(const std::string& prog,
                             const std::vector<std::string>& args,
                             const std::string& version_key = "");

// Perl: program_version_cmd() — run "prog --version" style
std::string program_version_cmd(const std::string& prog,
                                const std::string& arg = "--version");

// Perl: program_version_pkg() — query package manager for version
std::string program_version_pkg(const std::string& prog);

// Perl: awk($n, $line) — return nth whitespace-delimited field (1-indexed)
std::string awk_field(const std::string& line, int field_n);

// Perl: get_pms() — returns list of detected package managers
std::vector<std::string> get_package_managers(const std::vector<std::string>& paths = {});

// Perl: get_defined() — return first non-empty string from a list
template<typename... Args>
std::string get_defined(Args&&... args) {
    for (const std::string& s : {std::string(std::forward<Args>(args))...}) {
        if (!s.empty()) return s;
    }
    return "";
}

}  // namespace inxi::utils
