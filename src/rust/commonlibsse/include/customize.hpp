//
// Created by m on 10/23/2022.
//

#ifndef SKYRIMOUTFITSYSTEMSE_SRC_RUST_COMMONLIBSSE_SRC_CUSTOMIZE_H
#define SKYRIMOUTFITSYSTEMSE_SRC_RUST_COMMONLIBSSE_SRC_CUSTOMIZE_H

#if RUST_DEFINES

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <REL/Relocation.h>

namespace RE {
    TESObjectARMO* ResolveARMOFormID(FormID id);
    FormID TESDataHandler_LookupFormIDRawC(TESDataHandler* dh, FormID raw_form_id, const char* mod_name);
}

#endif

#endif //SKYRIMOUTFITSYSTEMSE_SRC_RUST_COMMONLIBSSE_SRC_CUSTOMIZE_H
