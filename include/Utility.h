#pragma once

#include <string>

std::string GetRuntimeName();

const std::string& GetRuntimeDirectory();

class Settings {
public:
    Settings();
    ~Settings();
    INIReader reader;
    static INIReader* Instance();
};

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
