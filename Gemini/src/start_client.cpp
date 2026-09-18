// start_client.cpp
#include "start_client.h"
#include "utils/file_utils.h"
#include "utils/process_utils.h"
#include "utils/string_utils.h"
#include <cstdlib>
#include <unistd.h>

namespace inxi {

StartClient::StartClient(GlobalState& state) : state_(state) {}

void StartClient::detect() {
    pid_t ppid = getppid();
    std::string cmd = get_cmdline(ppid);
    std::string client_name = get_client_name();

    state_.client.name = client_name;
    state_.client.version = get_client_version(client_name);

    if (state_.client.name.empty()) {
        const char* term = std::getenv("TERM");
        state_.client.name = term ? term : "terminal";
    }

    if (check_konvi(cmd)) {
        set_konvi_data();
    }
}

std::string StartClient::get_cmdline(int pid) {
    std::string path = "/proc/" + std::to_string(pid) + "/cmdline";
    std::string raw = utils::read_file_str(path);
    // cmdline has \0 as delimiters
    for (char& c : raw) {
        if (c == '\0') c = ' ';
    }
    return utils::trim(raw);
}

std::string StartClient::get_client_name() {
    pid_t ppid = getppid();
    std::string comm_path = "/proc/" + std::to_string(ppid) + "/comm";
    std::string comm = utils::read_file_str(comm_path);
    if (!comm.empty()) return comm;

    const char* term = std::getenv("TERM");
    if (term) return term;
    return "";
}

std::string StartClient::get_client_version(const std::string& client_name) {
    if (client_name.empty()) return "";
    return utils::program_version_cmd(client_name);
}

bool StartClient::script_client(const std::string& cmdline) {
    return (cmdline.find("python") != std::string::npos ||
            cmdline.find("perl") != std::string::npos);
}

bool StartClient::check_konvi(const std::string& cmdline) {
    return (cmdline.find("konversation") != std::string::npos);
}

void StartClient::set_konvi_data() {
    state_.client.konvi = true;
}

}  // namespace inxi
