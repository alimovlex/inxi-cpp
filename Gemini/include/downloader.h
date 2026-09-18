// downloader.h
// Network download helper (curl/wget/fetch).
#pragma once
#include "global_state.h"
#include <string>

namespace inxi {

class Downloader {
public:
    explicit Downloader(GlobalState& state);

    // Downloads content from url into a string
    std::string download_string(const std::string& url);

    // Downloads content from url directly to a file
    bool download_file(const std::string& url, const std::string& destination);

private:
    GlobalState& state_;
};

}  // namespace inxi
