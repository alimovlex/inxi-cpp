// global_state.h
// Central repository for all global state that the Perl version stored in
// package-level variables. In C++ these live in a singleton GlobalState struct
// that is passed by reference into every subsystem.
//
// Original Perl globals mapped here:
//   %alerts, %build_prop, %client, %colors, %cpuinfo_machine, %disks_bsd,
//   %dboot, %devices, %dl, %dmmapper, %force, %loaded, %mapper,
//   %program_values, %risc, %service_tool, %show, %sysctl, %system_files,
//   %usb, %windows
//   @app, @cpuinfo, @dmi, @ifs, @ifs_bsd, @paths, @ps_aux, @ps_cmd,
//   @ps_gui, @sensors_exclude, @sensors_use, @uname
//   @btrfs_raid, @glabel, @labels, @lsblk, @lvm, @lvm_raid, @md_raid,
//   @partitions, @proc_partitions, @raw_logical, @soft_raid, @swaps,
//   @uuids, @zfs_raid
//
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace inxi {

// -----------------------------------------------------------------------
// Alert record (per-tool availability status)
// -----------------------------------------------------------------------
struct Alert {
    std::string action;   // "use" | "missing" | "permissions" | "platform" |
                          // "smbios" | "no-data" | "unknown-error"
    std::string path;     // full path if found
    std::string message;  // human-readable reason
};

// -----------------------------------------------------------------------
// Download helper state
// -----------------------------------------------------------------------
struct DlState {
    std::string dl;    // preferred downloader name: "curl"|"wget"|"fetch"|"tiny"
    bool curl  = true;
    bool fetch = true;
    bool tiny  = true;
    bool wget  = true;
};

// -----------------------------------------------------------------------
// RISC architecture flags
// -----------------------------------------------------------------------
struct RiscFlags {
    bool arm   = false;
    bool mips  = false;
    bool ppc   = false;
    bool riscv = false;
    bool sparc = false;
    std::string id;       // "arm"|"mips"|"ppc"|"riscv"|"sparc"|""
};

// -----------------------------------------------------------------------
// Windows / compatibility layer flags
// -----------------------------------------------------------------------
struct WindowsFlags {
    bool cygwin = false;
    bool wsl    = false;
};

// -----------------------------------------------------------------------
// Display size state
// -----------------------------------------------------------------------
struct SizeState {
    int console    = 80;
    int indent     = 11;
    int indents    = 2;
    int irc        = 100;
    int lines      = 1;
    int max_cols   = 0;
    int max_lines  = 0;
    int max_wrap   = 110;
    int no_display = 100;
    int term_cols  = 80;
    int term_lines = 40;
    // set at runtime
    int max_cols_basic = 0;
};

// -----------------------------------------------------------------------
// Color scheme state
// -----------------------------------------------------------------------
struct ColorState {
    int  scheme  = 2;     // default scheme index
    int  def     = 2;
    // Named ANSI escape sequences, filled by Colors::set()
    std::unordered_map<std::string, std::string> ansi;
    // Active role → escape mapping (c1/c2/c3)
    std::string c1, c2, c3;
    std::string reset;
};

// -----------------------------------------------------------------------
// Separator strings (IRC vs console)
// -----------------------------------------------------------------------
struct SepState {
    std::string s1_irc     = ":";
    std::string s1_console = ":";
    std::string s2_irc     = "";
    std::string s2_console = ":";
    // Active (set by set_sep())
    std::string s1, s2;
};

// -----------------------------------------------------------------------
// Client (IRC / terminal) identification
// -----------------------------------------------------------------------
struct ClientState {
    bool console_irc  = false;
    bool dcop         = false;
    bool qdbus        = false;
    bool konvi        = false;
    std::string name;
    std::string name_print;
    std::string su_start;
    std::string version;
    std::string whoami;
    int  test_konvi   = 0;
};

// -----------------------------------------------------------------------
// Show flags (which info sections to output)
// Maps to $show{...} in Perl.
// -----------------------------------------------------------------------
struct ShowFlags {
    bool audio            = false;
    bool battery          = false;
    bool bluetooth        = false;
    bool cpu              = false;
    bool disk             = false;
    bool disk_total       = false;
    bool display          = false;
    bool graphics         = false;
    bool host             = false;
    bool info             = false;
    bool ip               = false;
    bool machine          = false;
    bool memory           = false;
    bool network          = false;
    bool network_advanced = false;
    bool partition        = false;
    bool process          = false;
    bool raid             = false;
    bool recommends       = false;
    bool repo             = false;
    bool sensor           = false;
    bool swap             = false;
    bool system           = false;
    bool unmounted        = false;
    bool usb              = false;
    bool weather          = false;
    std::string partition_sort = "id";  // sort order
};

// -----------------------------------------------------------------------
// Force flags (user overrides)
// Maps to %force{...} in Perl.
// -----------------------------------------------------------------------
struct ForceFlags {
    bool no_doas  = false;
    bool no_sudo  = false;
    bool pci      = false;
    bool sensors  = false;
    bool usb      = false;
};

// -----------------------------------------------------------------------
// Feature use-flags (capability gates)
// -----------------------------------------------------------------------
struct UseFlags {
    bool block_tool  = true;
    bool bsd_part    = true;
    bool btrfs       = true;
    bool dmidecode   = true;
    bool logical     = true;
    bool mdadm       = true;
    bool pci         = true;
    bool smartctl    = true;
    bool sysctl      = true;
    bool update      = true;
    bool usb         = true;
    bool weather     = true;
};

// -----------------------------------------------------------------------
// System-file path table
// Maps to %system_files{...} in Perl.
// -----------------------------------------------------------------------
struct SystemFiles {
    std::string asound_cards;
    std::string asound_modules;
    std::string asound_version;
    std::string dmesg_boot;
    std::string proc_cmdline;
    std::string proc_cpuinfo;
    std::string proc_mdstat;
    std::string proc_meminfo;
    std::string proc_modules;
    std::string proc_mounts;
    std::string proc_partitions;
    std::string proc_scsi;
    std::string proc_version;
    std::string xorg_log;
};

// -----------------------------------------------------------------------
// Debugger state
// -----------------------------------------------------------------------
struct DebuggerState {
    int  level    = 0;
    bool sys      = true;
    bool log      = false;
    bool log_colors  = false;
    bool log_full    = false;
    std::string log_file;
    std::vector<bool> flags; // indexed $dbg[N] from Perl
};

// -----------------------------------------------------------------------
// Program value entry
// -----------------------------------------------------------------------
struct ProgramValue {
    std::string name;
    std::string path;
    std::string version;
    std::vector<std::string> version_args;
    std::string version_key;
};

// -----------------------------------------------------------------------
// GlobalState — the single top-level state object
// -----------------------------------------------------------------------
struct GlobalState {
    // Self identity
    std::string self_name    = "inxi";
    std::string self_version = "3.3.31";
    std::string self_date    = "2023-11-02";
    std::string self_patch   = "00";
    std::string self_path;

    // User dirs
    std::string user_config_dir;
    std::string user_config_file;
    std::string user_data_dir;
    std::string fake_data_dir;

    // System identification
    std::string os;           // "linux"|"freebsd"|"openbsd"|"darwin" …
    std::string bsd_type;     // "" on Linux
    std::string cpu_arch;
    int         bits_sys = 0; // 32 or 64
    std::vector<std::string> uname_vals; // [0..4] sysname nodename release version machine

    // Special OS environments
    bool b_android = false;

    // Runtime booleans (Perl $b_xxx)
    bool b_irc     = false;
    bool b_display = false;
    bool b_root    = false;
    bool b_admin   = false;
    bool b_busybox_ps = false;

    // Device / VM strings
    std::string device_vm;
    std::string language;
    std::string pci_tool;     // "lspci"|"pciconf"|"pcictl"|"pcidump"

    // Network
    std::string wan_url;
    std::string wl_compositors;

    // Output
    int         extra        = 0;    // verbosity: 0–3
    bool        filter       = false;// -z / --filter: privacy filter
    std::string filter_str   = "<filter>";
    std::string output_file;
    std::string output_type  = "screen";
    int         prefix       = 0;

    // Timing
    double cpu_sleep   = 0.35;
    int    dl_timeout  = 4;
    int    limit       = 10;
    int    ps_cols     = 0;
    int    ps_count    = 5;
    int    sensors_cpu_nu = 0;
    int    weather_source = 100;
    std::string weather_unit = "mi";

    // Tools
    std::string display_var;
    std::string display_opt;
    std::string sudoas;
    std::string ftp_alt;

    // Sub-states
    Alert                                          alert_placeholder; // unused sentinel
    std::unordered_map<std::string, Alert>         alerts;
    ClientState                                    client;
    ColorState                                     colors;
    DebuggerState                                  debugger;
    DlState                                        dl;
    ForceFlags                                     force;
    RiscFlags                                      risc;
    SepState                                       sep;
    ShowFlags                                      show;
    SizeState                                      size;
    SystemFiles                                    system_files;
    UseFlags                                       use;
    WindowsFlags                                   windows;

    // Flat string maps (Perl hash → map)
    std::unordered_map<std::string, std::string>   build_prop;
    std::unordered_map<std::string, std::string>   cpuinfo_machine;
    std::unordered_map<std::string, std::string>   devices;
    std::unordered_map<std::string, std::string>   dmmapper;
    std::unordered_map<std::string, std::string>   loaded;
    std::unordered_map<std::string, std::string>   mapper;
    std::unordered_map<std::string, std::string>   service_tool;
    std::unordered_map<std::string, int>            sysctl;
    std::unordered_map<std::string, std::string>   usb_map;
    std::unordered_map<std::string, std::string>   program_values;

    // System arrays
    std::vector<std::string> app;
    std::vector<std::string> cpuinfo;
    std::vector<std::string> dmi;
    std::vector<std::string> ifs;
    std::vector<std::string> ifs_bsd;
    std::vector<std::string> paths;
    std::vector<std::string> ps_aux;
    std::vector<std::string> ps_cmd;
    std::vector<std::string> ps_gui;
    std::vector<std::string> sensors_exclude;
    std::vector<std::string> sensors_use;

    // Disk / Logical / Partition / RAID arrays
    std::vector<std::string> btrfs_raid;
    std::vector<std::string> glabel;
    std::vector<std::string> labels;
    std::vector<std::string> lsblk;
    std::vector<std::string> lvm;
    std::vector<std::string> lvm_raid;
    std::vector<std::string> md_raid;
    std::vector<std::string> partitions;
    std::vector<std::string> proc_partitions;
    std::vector<int>         raw_logical = {0, 0, 0};
    std::vector<std::string> soft_raid;
    std::vector<std::string> swaps;
    std::vector<std::string> uuids;
    std::vector<std::string> zfs_raid;

    // Disks BSD
    std::unordered_map<std::string, std::string> disks_bsd;
    // dboot map
    std::unordered_map<std::string, std::string> dboot;
};

}  // namespace inxi
