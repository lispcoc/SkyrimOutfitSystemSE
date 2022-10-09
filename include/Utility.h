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
