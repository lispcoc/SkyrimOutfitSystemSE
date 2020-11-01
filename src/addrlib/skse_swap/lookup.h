//
// Created by m on 11/1/2020.
//

#ifndef SKYRIMOUTFITSYSTEMSE_LOOKUP_H
#define SKYRIMOUTFITSYSTEMSE_LOOKUP_H

// #include "addrlib_offsets.h"
#include "versiondb.h"

extern VersionDb* s_versionDb;

// template<uintptr_t offset> inline uintptr_t offset_lookup() {
//     if (!s_versionDb) {
//         s_versionDb = new VersionDb();
//         s_versionDb->Load();
//     }
//     constexpr uintptr_t id = Offsets::addrMap.at(offset);
//     return s_versionDb->FindOffsetById(id);
// }

inline uintptr_t runtime_offset_lookup(uintptr_t offset) {
    if (!s_versionDb) {
        s_versionDb = new VersionDb();
        s_versionDb->Load();
    }
    uintptr_t id;
    bool found = s_versionDb->FindIdByOffset(offset, id);
    if (!found) {
        char error[100];
        sprintf_s(error, "Could not find offset %lli", offset);
        throw std::runtime_error(error);
    }
    uintptr_t result;
    s_versionDb->FindOffsetById(id, result);
    return result;
}


#endif //SKYRIMOUTFITSYSTEMSE_LOOKUP_H
