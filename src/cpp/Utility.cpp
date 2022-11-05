//
// Created by m on 8/22/2022.
//

#include "Utility.h"

#include "SKSE/SKSE.h"

#undef GetModuleFileName
#undef GetModuleHandle

std::unique_ptr<std::string> GetRuntimePath() {
    static char appPath[4096] = {0};

    if (appPath[0])
        return std::make_unique<std::string>(appPath);

    if (!SKSE::WinAPI::GetModuleFileName(SKSE::WinAPI::GetModuleHandle((const char*) nullptr), appPath, sizeof(appPath))) {
        SKSE::stl::report_and_fail("Failed to get runtime path");
    }

    return std::make_unique<std::string>(appPath);
}

std::unique_ptr<std::string> GetRuntimeName() {
    std::string appPath = *GetRuntimePath();

    std::string::size_type slashOffset = appPath.rfind('\\');
    if (slashOffset == std::string::npos)
        return std::make_unique<std::string>(appPath);

    return std::make_unique<std::string>(appPath.substr(slashOffset + 1));
}

std::unique_ptr<std::string> GetRuntimeDirectory() {
    static std::string s_runtimeDirectory;

    if (s_runtimeDirectory.empty()) {
        std::string runtimePath = *GetRuntimePath();

        // truncate at last slash
        std::string::size_type lastSlash = runtimePath.rfind('\\');
        if (lastSlash != std::string::npos)// if we don't find a slash something is VERY WRONG
        {
            s_runtimeDirectory = runtimePath.substr(0, lastSlash + 1);
        } else {
            LOG(critical, "no slash in runtime path? (%s)", runtimePath.c_str());
        }
    }

    return std::make_unique<std::string>(s_runtimeDirectory);
}
