#include "inxi/options.hpp"
#include "inxi/output.hpp"
#include "inxi/report.hpp"
#include "inxi/util.hpp"

#include <iostream>

int main(int argc, char** argv) {
    auto opt = inxi::parse_args(argc, argv);
    if (opt.show_help) {
        inxi::print_help();
        return 0;
    }
    if (opt.show_version || opt.show_version_short) {
        inxi::print_version(opt.show_version_short);
        return 0;
    }
    if (opt.show_recommends) {
        inxi::print_recommends();
        return 0;
    }
    if (opt.show_configs) {
        std::cout << "Configuration files (Perl inxi compatible locations, not all keys honored):\n"
                  << "  /etc/inxi.conf\n"
                  << "  /etc/inxi.d/\n"
                  << "  " << inxi::env("HOME") << "/.config/inxi.conf\n"
                  << "  " << inxi::env("HOME") << "/.inxi/inxi.conf\n";
        return 0;
    }
    auto report = inxi::build_report(opt);
    std::cout << inxi::render(report, opt);
    return 0;
}
