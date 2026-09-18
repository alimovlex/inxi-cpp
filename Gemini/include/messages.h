// messages.h
// Error, warning, and informational messages for inxi.
#pragma once
#include "global_state.h"
#include <string>

namespace inxi {

enum class MessageType { Info, Warning, Error, Alert };

class Messages {
public:
    explicit Messages(GlobalState& state) : state_(state) {}

    void print(MessageType type, const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);
    void info(const std::string& msg);

private:
    GlobalState& state_;
};

void print_message(GlobalState& state, MessageType type, const std::string& msg);

}  // namespace inxi
