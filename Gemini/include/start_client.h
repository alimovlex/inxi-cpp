// start_client.h
// Detect and record the IRC client or terminal context.
// Perl source: package StartClient (lines ~6377–6737)
//
// Perl subs mapped:
//   StartClient::set()              → StartClient::detect()
//   StartClient::get_client_name()  → StartClient::get_client_name()
//   StartClient::get_client_version()→ StartClient::get_client_version()
//   StartClient::get_cmdline()      → StartClient::get_cmdline()
//   StartClient::perl_python_client()→StartClient::script_client()
//   StartClient::check_modern_konvi()→StartClient::check_konvi()
//   StartClient::set_konvi_data()   → StartClient::set_konvi_data()
#pragma once
#include "global_state.h"
#include <string>

namespace inxi {

class StartClient {
public:
    explicit StartClient(GlobalState& state);

    // Perl: set() — detect client, fill state.client.*
    void detect();

private:
    GlobalState& state_;

    // Perl: get_client_name() — resolve the IRC client name
    std::string get_client_name();

    // Perl: get_client_version() — resolve the IRC client version
    std::string get_client_version(const std::string& client_name);

    // Perl: get_cmdline() — read /proc/<ppid>/cmdline
    std::string get_cmdline(int pid);

    // Perl: perl_python_client() — detect script-based IRC clients
    bool script_client(const std::string& cmdline);

    // Perl: check_modern_konvi() — detect konversation ≥ 1.6
    bool check_konvi(const std::string& cmdline);

    // Perl: set_konvi_data() — populate konversation-specific client data
    void set_konvi_data();
};

}  // namespace inxi
