//
// Created by m on 11/1/2020.
//

#ifndef SKYRIMOUTFITSYSTEMSE_LOOKUP_H
#define SKYRIMOUTFITSYSTEMSE_LOOKUP_H

#include "addrlib_offsets.h"
#include "versiondb.h"

extern VersionDb* s_versionDbCurrent;

// template<uintptr_t offset> inline uintptr_t offset_lookup() {
//     if (!s_versionDb) {
//         s_versionDb = new VersionDb();
//         s_versionDb->Load();
//     }
//     constexpr uintptr_t id = Offsets::addrMap.at(offset);
//     return s_versionDb->FindOffsetById(id);
// }

inline uintptr_t runtime_offset_lookup(uintptr_t offset) {
    if (!s_versionDbCurrent) {
        s_versionDbCurrent = new VersionDb();
        s_versionDbCurrent->Load();
    }
    auto iter = Offsets::addrMap.find(offset);
    if (iter == Offsets::addrMap.end()) {
        char error[100];
        sprintf_s(error, "Could not find offset %p", (void*) offset);
        _ERROR("Could not find offset %p", (void*) offset);
        _ERROR("I AM CRASHING THE GAME");
        int* p = 0;
        *p;
        // throw std::runtime_error(error);
    } else {
        // _MESSAGE("Mapped offset %p", offset);
    }
    uintptr_t result;
    s_versionDbCurrent->FindOffsetById(iter->second, result);
    return result;
}


#endif //SKYRIMOUTFITSYSTEMSE_LOOKUP_H
