# inxi-cpp

A partial C++17 port of [inxi](https://github.com/smxi/inxi), the Perl
system-information tool. This is a fresh implementation of inxi's most-used
report sections, not a line-by-line translation — see **Scope** and
**Why not a full port** below for why.

## Build

```sh
make
./inxi-cpp
```

No dependencies beyond a C++17 compiler and the standard library
(`std::filesystem` is used throughout). Tested with g++ 13 on Linux.

`make install` installs to `/usr/local/bin` (set `PREFIX=` or `DESTDIR=` to
change that).

## Usage

```
inxi-cpp                 short one-line summary (default, no args)
inxi-cpp -F               full report: all sections below except -m
inxi-cpp -S               System: host, kernel, arch, desktop, distro
inxi-cpp -M               Machine: DMI system/board/BIOS info
inxi-cpp -C               CPU: model, cores, speed, cache
inxi-cpp -m               Memory: RAM total/used/available
inxi-cpp -G               Graphics: GPU device(s), display server
inxi-cpp -N               Network: interfaces, driver, state, MAC
inxi-cpp -D               Drives: physical block devices
inxi-cpp -P               Partition: mounted filesystems, usage
inxi-cpp -j               Swap: swap devices/files in use
inxi-cpp -I               Info: uptime, processes, packages, shell

Modifiers: -f (full CPU flags, triggers -C) -z (mask MAC addresses)
           -c 0|1 (force color off/on) -h/--help  -v/--version
```

Flags combine like inxi's do: `inxi-cpp -SCMm` shows four sections in a
fixed order regardless of the order you type the letters.

## Scope

This port covers the sections most people actually run inxi for, each
reading the same underlying sources inxi does:

| Section | Source(s) |
|---|---|
| System (`-S`) | `uname()`, `/etc/os-release`, `$XDG_CURRENT_DESKTOP` |
| Machine (`-M`) | `/sys/class/dmi/id/*` |
| CPU (`-C`) | `/proc/cpuinfo`, `/sys/devices/system/cpu/cpu0/{cache,cpufreq}` |
| Memory (`-m`) | `/proc/meminfo` |
| Graphics (`-G`) | `lspci` (device list), `$WAYLAND_DISPLAY`/`$DISPLAY`, `xrandr` (resolution) |
| Network (`-N`) | `/sys/class/net/*` |
| Drives (`-D`) | `/sys/block/*` |
| Partition (`-P`) | `/proc/mounts` + `statvfs()` |
| Swap (`-j`) | `/proc/swaps` |
| Info (`-I`) | `/proc/uptime`, `/proc/[0-9]*`, `dpkg`/`rpm`/`pacman`/`apk`, `$SHELL` |

**Not yet ported:** Audio (`-A`), Bluetooth (`-E`), Battery (`-B`), RAID
(`-R`), Sensors (`-s`), Weather (`-w`/`-W`), USB (`-J`), distro repositories
(`-r`), LVM/logical devices (`-L`), process listing (`-t`), WAN IP (`-i`),
the `-v 0-8` verbosity ladder, PCI slots (`--slots`), and JSON/extra-data
output (`-x`/`-xx`/`-xxx`, `-J`). Real inxi also has a CPU model→codename
database and an OUI vendor lookup for drive/network hardware; this port
reports the raw strings the kernel gives it instead.

Real inxi also degrades to `dmidecode` (as root) when `/sys/class/dmi/id`
isn't populated. This port only reads `/sys`, so Machine output will be
sparser on older kernels or in some VMs/containers.

## Why not a full port

Original inxi is a single ~36,700-line Perl script covering roughly 50
command-line flags, built up over about 20 years, including hardware
ID/vendor/codename lookup tables, distro detection for dozens of
distributions, and BSD/Android/WSL-specific code paths. That's a
multi-month professional effort to port faithfully. This port instead
re-implements the core report sections cleanly, in a structure where
adding the next one is a small, contained change (see below) rather than
starting over.

## Adding a new section

Each section is one `.h`/`.cpp` pair in `src/` exposing a single
`print*Section(std::ostream&, bool color)` function (see
`graphics_module.*` for a short example). To add one:

1. Create `src/<name>_module.h` / `.cpp` following that pattern.
2. Use the helpers in `common.h` (`readFile`, `readLines`, `runCommand`,
   `humanSize`, `SectionPrinter`, ...) to gather and print the data.
3. Wire it into `main.cpp`: add a flag letter to the `switch`, and a
   `printXSection(...)` call in the output block.

## License

inxi itself is GPLv3. This is a fresh, from-scratch implementation of
similar functionality and output shape (not a copy or mechanical
translation of inxi's Perl source), written by reading `/proc`/`/sys` and
documented tool output directly. No GPLv3 source text is included here.
If you plan to distribute this, it's worth doing your own license review
for your use case — this note is informational, not legal advice — and
consider not calling your build "inxi" to avoid confusion with the
original project.
