//
// Created by m on 10/23/2022.
//

#ifndef SKYRIMOUTFITSYSTEMSE_SRC_RUST_COMMONLIBSSE_SRC_CUSTOMIZE_H
#define SKYRIMOUTFITSYSTEMSE_SRC_RUST_COMMONLIBSSE_SRC_CUSTOMIZE_H

#if RUST_DEFINES

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

namespace RE {
    using BipedObjectSlot = BIPED_MODEL::BipedObjectSlot;
    TESObjectARMO* ResolveARMOFormID(FormID id);
}
#endif

#endif //SKYRIMOUTFITSYSTEMSE_SRC_RUST_COMMONLIBSSE_SRC_CUSTOMIZE_H
