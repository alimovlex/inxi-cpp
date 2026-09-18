// downloader.cpp
#include "downloader.h"
#include "utils/file_utils.h"
#include "utils/process_utils.h"
#include <cstdlib>

namespace inxi {

Downloader::Downloader(GlobalState& state) : state_(state) {}

std::string Downloader::download_string(const std::string& url) {
    std::string curl_path = utils::check_program("curl");
    if (!curl_path.empty()) {
        std::string cmd = curl_path + " -sSL --max-time " + std::to_string(state_.dl_timeout) + " " + url;
        auto lines = utils::run_capture(cmd);
        std::string result;
        for (const auto& l : lines) result += l + "\n";
        return result;
    }
    std::string wget_path = utils::check_program("wget");
    if (!wget_path.empty()) {
        std::string cmd = wget_path + " -qO- -T " + std::to_string(state_.dl_timeout) + " " + url;
        auto lines = utils::run_capture(cmd);
        std::string result;
        for (const auto& l : lines) result += l + "\n";
        return result;
    }
    return "";
}

bool Downloader::download_file(const std::string& url, const std::string& destination) {
    std::string content = download_string(url);
    if (content.empty()) return false;
    return utils::write_file(destination, content);
}

}  // namespace inxi
