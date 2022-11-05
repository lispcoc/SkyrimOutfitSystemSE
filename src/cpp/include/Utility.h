#pragma once

#include <string>

std::unique_ptr<std::string> GetRuntimeName();

std::unique_ptr<std::string> GetRuntimeDirectory();

class LogExit {
public:
    std::string_view m_string;
    LogExit(std::string_view name) : m_string(name) {
        LOG(trace, "Enter {}", m_string);
    };
    ~LogExit() {
        LOG(trace, "Exit {}", m_string);
    };
};
