#pragma once
// common.h -- shared helpers used across every report module.
//
// inxi is a single ~36,700 line Perl script; this port splits the same
// functionality into one small module per report section. This header
// holds the plumbing every module needs so the module files themselves
// can stay focused on "what data to gather", not "how to read a file".

#include <string>
#include <vector>
#include <cstdint>
#include <ostream>

namespace util {

// ---- Reading from the OS ---------------------------------------------
// inxi gets almost all of its data by reading /proc and /sys files, and
// shelling out to standard tools (lspci, lsblk, etc.) when there's no
// kernel interface for something. These helpers mirror that approach.

std::string readFile(const std::string& path);                // "" if unreadable
std::string readFirstLine(const std::string& path);            // trimmed first line, "" if unreadable
std::vector<std::string> readLines(const std::string& path);   // all lines, empty if unreadable
std::string runCommand(const std::string& cmd);                 // captured stdout, "" on failure
bool commandExists(const std::string& name);                    // true if found on PATH

// ---- String helpers -----------------------------------------------------
std::string trim(const std::string& s);
std::vector<std::string> split(const std::string& s, char delim);
bool startsWith(const std::string& s, const std::string& prefix);

// ---- Formatting, matching inxi's conventions -----------------------------
// inxi reports sizes as base-1024 with a unit suffix, e.g. "3.91 GiB".
std::string humanSize(double bytes, int decimals = 2);
// inxi's uptime style, e.g. "2d 3h 14m" or "0m".
std::string formatUptime(double seconds);
// "6.8%" style, returns "N/A" if whole <= 0.
std::string percentStr(double part, double whole, int decimals = 1);

// ---- Color / TTY ----------------------------------------------------------
bool stdoutIsTty();

enum class Color { Reset, Header, Label, Value, Warn };
std::string paint(const std::string& text, Color c, bool enabled);

// A tiny helper that prints sections the way inxi does:
//
//   System:
//     Host: myhost Kernel: 6.8.0-31-generic arch: x86_64 bits: 64
//     Distro: Ubuntu 24.04.4 LTS (Noble Numbat)
//
// Usage:
//   SectionPrinter p(std::cout, color);
//   p.header("System");
//   p.beginLine();
//   p.field("Host", host);
//   p.field("Kernel", release);
//   p.endLine();
class SectionPrinter {
public:
    SectionPrinter(std::ostream& os, bool color) : os_(os), color_(color) {}

    void header(const std::string& name);   // prints "Name:\n"
    void beginLine(int indent = 2);          // starts a new indented line
    void field(const std::string& label, const std::string& value); // " Label: value"
    void raw(const std::string& text);       // append text with no label
    void endLine();                          // newline

private:
    std::ostream& os_;
    bool color_;
    bool lineOpen_ = false;
    bool firstFieldOnLine_ = true;
};

} // namespace util
