# inxi-cpp

C++ rewrite of [inxi](https://github.com/smxi/inxi) / [codeberg.org/smxi/inxi](https://codeberg.org/smxi/inxi), the CLI system information tool.

Perl inxi is a single ~36k-line script. This project reimplements the Linux report pipeline in C++20: the same short flags, verbosity levels, and `Key: value` layout, reading `/proc`, `/sys`, DMI, PCI/USB ID databases, and optional helpers (`glxinfo`, `xrandr`, `dmidecode`, `curl`).

It is **not** a line-by-line translation. BSD, Cygwin, IRC client quirks, the Perl debugger, EDID parsers, and many `-a` admin corners are incomplete. Linux reports for the common flags (`-b`, `-F`/`-e`, `-v0..8`, `-CGmn`, JSON/XML) are the supported core.

## Build

```sh
cmake -S . -B build
cmake --build build -j
./build/inxi
./build/inxi -b -c0
./build/inxi -Fxxz -c0
```

No extra libraries. C++20 compiler required (`g++` or `clang++`).

## License

GNU GPL v3, same as upstream inxi.
