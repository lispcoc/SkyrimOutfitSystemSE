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

void check_file(std::map<unsigned long long, unsigned long long>& usedAddrToId, std::vector<std::tuple<std::string, std::string>>& failing, const std::filesystem::path& path, const std::map<unsigned long long, unsigned long long>& addrToId) {
    std::ifstream file(path);
    std::string line;
    std::regex hex_num_re("0[xX][0-9a-fA-F]{8}", std::regex_constants::ECMAScript);
    while (std::getline(file, line)) {
        std::sregex_iterator words_begin(line.begin(), line.end(), hex_num_re);
        for (std::sregex_iterator match = words_begin; match != std::sregex_iterator(); ++match) {
//            std::cout << match->str() << std::endl;
            uintptr_t match_int = std::stoull(match->str(), 0, 16);
            auto findMapping = addrToId.find(match_int);
            if (findMapping == addrToId.end()) {
                failing.emplace_back(path.string(), match->str());
            } else {
                usedAddrToId.emplace(*findMapping);
            }
        }
    }}

std::map<unsigned long long, unsigned long long> check_source_files(const char* path, const std::map<unsigned long long, unsigned long long>& addrToId) {
    using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;
    std::vector<std::tuple<std::string, std::string>> failing;
    std::map<unsigned long long, unsigned long long> usedAddrToId;
    try {
//        std::cout << "Checking source files in " << path << std::endl;
        for (const auto& dirEntry : recursive_directory_iterator(path)) {
//             std::cout << dirEntry.path() << std::endl;
             check_file(usedAddrToId, failing, dirEntry.path(), addrToId);
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
    return usedAddrToId;
}

int main(int argc, char *argv[]) {
    // Read CLI arguments and open the offset database.
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

    // Check all source files for which addresses are actually used.
    // Also dump the database to txt.
    std::cout << "Writing database dump." << std::endl;
    std::ofstream output_txt(std::string(argv[3]) + "\\offsets_" + argv[1] + ".txt");
    auto db = vdb.GetOffsetMap();
    char buffer[1000];
    std::map<unsigned long long, unsigned long long> reverse_map;
    for (auto record = db.cbegin(); record != db.cend(); ++record) {
        sprintf(buffer, "%lli %lli\n", record->second, record->first);
        output_txt << buffer;
        reverse_map[record->second] = record->first;
    }
    output_txt << std::endl;
    output_txt.close();
    std::cout << "Scanning SKSE source code." << std::endl;
    auto usedAddrToId = check_source_files(argv[4], reverse_map);
    // Write header
    std::cout << "Writing address map header." << std::endl;
    std::ofstream output(std::string(argv[3]) + "\\offsets_" + argv[1] + ".h");
    output << "#pragma once\n"
              "#include <skse64_common/eternal.hpp>"
              "\n"
              "namespace Offsets {\n"
              "\t// Generated code from addrheader\n"
              "\tconstexpr const auto addrMap = mapbox::eternal::map<uintptr_t, uintptr_t>({" << "\n";
    auto last_iter = --usedAddrToId.cend();
    for (auto record = usedAddrToId.cbegin(); record != usedAddrToId.cend(); ++record) {
        sprintf(buffer, "\t\t{ %lli, %lli }", record->first, record->second);
        if (record != last_iter) {
            output << buffer << "," << "\n";
        } else {
            output << buffer << "\n";
        }
    }
    output << "\t});\n"
              "}\n" << std::endl;
    output << "#define BUILT_AGAINST_SKYRIM_MAJOR " << major << "\n";
    output << "#define BUILT_AGAINST_SKYRIM_MINOR " << minor << "\n";
    output << "#define BUILT_AGAINST_SKYRIM_REVISION " << revision << "\n";
    output.close();
    std::cout << "Address map complete." << std::endl;
}
