// string_utils.h
// String manipulation utilities.
// Perl subs mapped:
//   trimmer()          → trim()
//   joiner()           → join()
//   lister()           → split_lines()
//   uniq()             → unique()
//   get_piece()        → get_piece()
//   clean_regex()      → clean_regex()
//   clean_characters() → clean_characters()
//   clean_arm()        → clean_arm()
//   clean_disk()       → clean_disk()
//   clean_dmi()        → clean_dmi()
//   clean_pci()        → clean_pci()
//   clean_pci_subsystem() → clean_pci_subsystem()
//   clean_unset()      → clean_unset()
//   is_int()           → is_int()
//   is_numeric()       → is_numeric()
//   is_hex()           → is_hex()
//   convert_hex()      → convert_hex()
//   compare_versions() → compare_versions()
//   regex_range()      → regex_range()
//   remove_duplicates()→ remove_duplicates()
//   translate_size()   → translate_size()
//   get_size()         → get_size()
//   filter()           → filter()
//   filter_partition() → filter_partition()
//   filter_pci_long()  → filter_pci_long()
#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace inxi::utils {

// Whitespace trimming
std::string trim(const std::string& s);
std::string trim_left(const std::string& s);
std::string trim_right(const std::string& s);

// Join vector of strings with a delimiter
std::string join(const std::vector<std::string>& v, const std::string& sep = " ");

// Split a string into lines (on \n)
std::vector<std::string> split_lines(const std::string& s);

// Split on arbitrary delimiter
std::vector<std::string> split(const std::string& s, char delim);
std::vector<std::string> split(const std::string& s, const std::string& delim);

// Unique-ify (preserve order, like Perl's uniq)
std::vector<std::string> unique(const std::vector<std::string>& v);

// Perl: get_piece($str, $n, $sep=" ") → nth word
std::string get_piece(const std::string& s, int n, const std::string& sep = " ");

// Cleaning helpers (remove junk from hardware strings)
std::string clean(std::string s);              // strip vendor/corp suffixes
std::string clean_characters(std::string s);  // strip non-printable
std::string clean_arm(std::string s);          // strip ARM model noise
std::string clean_disk(std::string s);         // strip disk name noise
std::string clean_dmi(std::string s);          // strip DMI noise strings
std::string clean_pci(std::string s);          // strip PCI name noise
std::string clean_pci_subsystem(std::string s);// strip PCI subsystem suffix
std::string clean_unset(const std::string& s); // return "" if value is "n/a" etc.
std::string clean_regex(std::string s);        // escape regex metacharacters
std::string strip_ansi(const std::string& s);  // remove ANSI escape sequences
size_t      visible_length(const std::string& s); // string length without ANSI codes

// Type checks
bool is_int(const std::string& s);
bool is_numeric(const std::string& s);
bool is_hex(const std::string& s);

// Conversions
uint64_t    convert_hex(const std::string& hex_str);
std::string translate_size(uint64_t bytes, int decimals = 2); // bytes → "2.5 GiB"
uint64_t    parse_size(const std::string& human);             // "2.5 GiB" → bytes

// Version comparison: returns -1 / 0 / +1
int compare_versions(const std::string& a, const std::string& b);

// Perl: regex_range() — expand "0-3" → ["0","1","2","3"]
std::vector<std::string> regex_range(const std::string& range_expr);

// Remove duplicates from a string list (stable)
std::vector<std::string> remove_duplicates(const std::vector<std::string>& v);

// Size string helpers
std::string get_size(uint64_t bytes);  // auto-choose unit

// Output filtering
std::string filter(const std::string& s);             // apply privacy filter
std::string filter_partition(const std::string& s);   // filter partition device
std::string filter_pci_long(const std::string& s);    // truncate long PCI names

// Case helpers
std::string to_lower(std::string s);
std::string to_upper(std::string s);

// printf-style formatting
std::string format(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

}  // namespace inxi::utils
