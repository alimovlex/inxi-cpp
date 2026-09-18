// math_utils.cpp
#include "utils/math_utils.h"
#include "utils/string_utils.h"
#include <cmath>
#include <iomanip>
#include <sstream>

namespace inxi::utils {

std::string format_bytes(uint64_t bytes, int decimals, bool binary) {
    const double step = binary ? 1024.0 : 1000.0;
    const char* units[] = {"B", binary ? "KiB" : "kB",
                                binary ? "MiB" : "MB",
                                binary ? "GiB" : "GB",
                                binary ? "TiB" : "TB",
                                binary ? "PiB" : "PB"};
    double val = static_cast<double>(bytes);
    int unit_idx = 0;
    while (val >= step && unit_idx < 5) {
        val /= step;
        unit_idx++;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimals) << val << " " << units[unit_idx];
    return oss.str();
}

std::string format_percent(double value, double total, int decimals) {
    if (total <= 0.0) return "0.0%";
    double pct = (value / total) * 100.0;
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimals) << pct << "%";
    return oss.str();
}

std::string format_frequency(double mhz, bool show_ghz) {
    std::ostringstream oss;
    if (show_ghz && mhz >= 1000.0) {
        oss << std::fixed << std::setprecision(2) << (mhz / 1000.0) << " GHz";
    } else {
        oss << std::fixed << std::setprecision(0) << mhz << " MHz";
    }
    return oss.str();
}

double round_to(double value, int decimals) {
    double factor = std::pow(10.0, decimals);
    return std::round(value * factor) / factor;
}

}  // namespace inxi::utils
