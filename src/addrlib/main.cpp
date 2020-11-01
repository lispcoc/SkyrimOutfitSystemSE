//
// Created by m on 11/1/2020.
//

#include <iostream>
#include <fstream>
#include <filesystem>
#include <direct.h>
#include <errno.h>
#include <string>
#include <regex>

#include "versiondb.h"

class TempCd {
private:
    char old_pwd[FILENAME_MAX];
    bool success;
public:
    TempCd(const char* newDir) {
        _getcwd( old_pwd, FILENAME_MAX );
        if(_chdir( newDir ) )
        {
            switch (errno)
            {
                success = false;
                case ENOENT:
                    printf( "Unable to locate the directory: %s\n", newDir );
                    break;
                case EINVAL:
                    printf( "Invalid buffer.\n");
                    break;
                default:
                    printf( "Unknown error.\n");
            }
        } else {
            success = true;
        }
    }
    ~TempCd() {
        if(_chdir( old_pwd ) )
        {
            switch (errno)
            {
                case ENOENT:
                    printf( "Unable to locate the directory: %s\n", old_pwd );
                    break;
                case EINVAL:
                    printf( "Invalid buffer.\n");
                    break;
                default:
                    printf( "Unknown error.\n");
            }
        }
    }
};

void check_file(std::vector<std::tuple<std::string, std::string>>& failing, const std::filesystem::path& path, const std::map<unsigned long long, unsigned long long>& addrToId) {
    std::ifstream file(path);
    std::string line;
    std::regex hex_num_re("0[xX][0-9a-fA-F]{8}", std::regex_constants::ECMAScript);
    while (std::getline(file, line)) {
        std::sregex_iterator words_begin(line.begin(), line.end(), hex_num_re);
        for (std::sregex_iterator match = words_begin; match != std::sregex_iterator(); ++match) {
//            std::cout << match->str() << std::endl;
            uintptr_t match_int = std::stoull(match->str(), 0, 16);
            if (addrToId.find(match_int) == addrToId.end()) {
                failing.emplace_back(path.string(), match->str());
            }
        }
    }}

void check_source_files(const char* path, const std::map<unsigned long long, unsigned long long>& addrToId) {
    using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;
    std::vector<std::tuple<std::string, std::string>> failing;
    try {
//        std::cout << "Checking source files in " << path << std::endl;
        for (const auto& dirEntry : recursive_directory_iterator(path)) {
//             std::cout << dirEntry.path() << std::endl;
             check_file(failing, dirEntry.path(), addrToId);
        }
    } catch (std::filesystem::filesystem_error& e) {
        std::cerr << "Could not find " << path << std::endl;
        throw e;
    }
    if (!failing.empty()) {
        for (auto& result : failing) {
            std::cerr << "MISSING OFFSET " << std::get<1>(result) << " IN " << std::get<0>(result) << std::endl;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        std::cerr << "You need to pass the following: version address_library_location header_write_location skse_location" << std::endl;
        return 1;
    }
    auto vdb = VersionDb();
    int major, minor, revision;
    if (sscanf_s(argv[1], "%i_%i_%i", &major, &minor, &revision) != 3) {
        std::cerr << "Invalid version string passed." << std::endl;
        return 2;
    }
    printf("Loading for Skyrim %i.%i.%i\n", major, minor, revision);

    // Temporarily change directory so that AddressLib can load DB
    {
        auto cd = TempCd(argv[2]);
        if (!vdb.Load(major, minor, revision, 0)) {
            std::cerr << "Address Library could not load database." << std::endl;
            return 3;
        } else {
            std::cout << "Address Library loaded database." << std::endl;
        }
    }
    std::ofstream output(std::string(argv[3]) + "\\offsets_" + argv[1] + ".h");
    std::ofstream output_txt(std::string(argv[3]) + "\\offsets_" + argv[1] + ".txt");
    auto db = vdb.GetOffsetMap();
    output << "#pragma once\n"
              "#include <skse64_common/eternal.hpp>"
              "\n"
              "namespace Offsets {\n"
              "\t// Generated code from addrheader\n"
              "\tconstexpr const auto addrMap = mapbox::eternal::map<uintptr_t, uintptr_t>({" << "\n";
    char buffer[1000];
    auto last_iter = --db.cend();
    std::map<unsigned long long, unsigned long long> reverse_map;
    for (auto record = db.cbegin(); record != db.cend(); ++record) {
        sprintf(buffer, "\t\t{ %lli, %lli }", record->second, record->first);
        if (record != last_iter) {
            output << buffer << "," << "\n";
        } else {
            output << buffer << "\n";
        }
        sprintf(buffer, "%lli %lli\n", record->second, record->first);
        output_txt << buffer;
        reverse_map[record->second] = record->first;
    }
    output << "\t});\n"
              "}\n" << std::endl;
    output.close();
    output_txt << std::endl;
    output_txt.close();
    std::cout << "Wrote header." << std::endl;
    check_source_files(argv[4], reverse_map);
    std::cout << "Done." << std::endl;
}
