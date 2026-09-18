// math_utils.h
// Math and sizing utilities.
// Formats bytes, percentages, rounding, and frequencies.
#pragma once
#include <string>
#include <cstdint>

namespace inxi::utils {

// Convert bytes to human-readable string: "107.65 GiB", "1.14 TiB", "512 MiB"
std::string format_bytes(uint64_t bytes, int decimals = 2, bool binary = true);

// Format percentage: "46.2%"
std::string format_percent(double value, double total, int decimals = 1);

// Parse human size like "500G", "1.5T", "512M" to bytes
uint64_t parse_size(const std::string& size_str);

// Format frequency in MHz or GHz: "2300 MHz" or "2.30 GHz"
std::string format_frequency(double mhz, bool show_ghz = false);

// Round to specified number of decimal places
double round_to(double value, int decimals);

}  // namespace inxi::utils
