//
// Created by m on 9/11/2022.
//

#ifndef SKYRIMOUTFITSYSTEMSE_SRC_HOOKING_HOOKS_AE_HPP
#define SKYRIMOUTFITSYSTEMSE_SRC_HOOKING_HOOKS_AE_HPP

namespace SKSE {
    class Trampoline;
}

namespace Hooking {
    extern SKSE::Trampoline* g_localTrampoline;
    extern SKSE::Trampoline* g_branchTrampoline;
}// namespace Hooking

extern "C" {
    void ApplyPlayerSkinningHooksSE();
    void ApplyPlayerSkinningHooksAE();
}

#endif//SKYRIMOUTFITSYSTEMSE_SRC_HOOKING_HOOKS_AE_HPP
