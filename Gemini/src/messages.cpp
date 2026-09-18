// messages.cpp
#include "messages.h"
#include <iostream>

namespace inxi {

void Messages::print(MessageType type, const std::string& msg) {
    switch (type) {
        case MessageType::Info:
            std::cout << "Info: " << msg << "\n";
            break;
        case MessageType::Warning:
            std::cerr << "Warning: " << msg << "\n";
            break;
        case MessageType::Error:
            std::cerr << "Error: " << msg << "\n";
            break;
        case MessageType::Alert:
            std::cerr << "Alert: " << msg << "\n";
            break;
    }
}

void Messages::warn(const std::string& msg) {
    print(MessageType::Warning, msg);
}

void Messages::error(const std::string& msg) {
    print(MessageType::Error, msg);
}

void Messages::info(const std::string& msg) {
    print(MessageType::Info, msg);
}

void print_message(GlobalState& state, MessageType type, const std::string& msg) {
    Messages msgs(state);
    msgs.print(type, msg);
}

}  // namespace inxi
