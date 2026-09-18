// check_recommends.h
// Checks for recommended tools and packages (-! 33 / -! commands).
#pragma once
#include "global_state.h"
#include <string>
#include <vector>

namespace inxi {

struct RecommendedTool {
    std::string name;
    std::string purpose;
    bool        installed = false;
    std::string package_name;
};

class CheckRecommends {
public:
    explicit CheckRecommends(GlobalState& state);

    std::vector<RecommendedTool> check();
    void print_report();

private:
    GlobalState& state_;
};

}  // namespace inxi
