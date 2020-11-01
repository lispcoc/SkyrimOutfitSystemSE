//
// Created by m on 11/1/2020.
//

#include <iostream>
#include <fstream>
#include <direct.h>
#include <errno.h>

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

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "You need to pass the following: version address_library_location header_write_location" << std::endl;
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
    auto db = vdb.GetOffsetMap();
    output << "#pragma once\n"
              "\n"
              "namespace Offsets {\n"
              "\t// Generated code from addrheader\n"
              "\tconstexpr const auto addrMap = mapbox::eternal::map<uintptr_t, uintptr_t>({" << "\n";
    char buffer[1000];
    auto last_iter = --db.cend();
    for (auto record = db.cbegin(); record != db.cend(); ++record) {
        sprintf(buffer, "\t\t{ %lli, %lli }", record->second, record->first);
        if (record != last_iter) {
            output << buffer << "," << "\n";
        } else {
            output << buffer << "\n";
        }
    }
    output << "\t});\n"
              "}\n" << std::endl;
    output.close();
    std::cout << "Done." << std::endl;
}
