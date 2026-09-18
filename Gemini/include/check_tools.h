// check_tools.h
// Verify availability and permissions of external tools.
// Perl source: package CheckTools (lines ~219–419)
//
// Perl subs mapped:
//   CheckTools::set()              → CheckTools::probe()
//   CheckTools::set_dmidecode()    → CheckTools::probe_dmidecode()
//   CheckTools::set_commands()     → CheckTools::build_command_table()
//   CheckTools::set_fake_bsd_tools()→ CheckTools::fake_bsd_tools()  [debug]
#pragma once
#include "global_state.h"

namespace inxi {

class CheckTools {
public:
    explicit CheckTools(GlobalState& state);

    // Perl: set() — probe all required tools and populate state.alerts
    void probe();

private:
    GlobalState& state_;

    // Perl: set_commands() — build the internal tool→test-spec table
    void build_command_table();

    // Perl: set_dmidecode() — special-case dmidecode error handling
    std::string probe_dmidecode(const std::vector<std::string>& output);

    // Perl: set_fake_bsd_tools() — override alerts for BSD fake-data debugging
    void fake_bsd_tools();
};

}  // namespace inxi
