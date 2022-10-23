//
// Created by m on 10/23/2022.
//

#ifndef SKYRIMOUTFITSYSTEMSE_SRC_RUST_COMMONLIBSSE_SRC_CUSTOMIZE_H
#define SKYRIMOUTFITSYSTEMSE_SRC_RUST_COMMONLIBSSE_SRC_CUSTOMIZE_H

#include <RE/Skyrim.h>
namespace RE {
    using BipedObjectSlot = BIPED_MODEL::BipedObjectSlot;
    PlayerCharacter* PlayerCharacter_GetSingleton() {
        return RE::PlayerCharacter::GetSingleton();
    }
}

#endif //SKYRIMOUTFITSYSTEMSE_SRC_RUST_COMMONLIBSSE_SRC_CUSTOMIZE_H
