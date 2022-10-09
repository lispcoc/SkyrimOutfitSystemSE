#pragma once

namespace SKSE {
    class Trampoline;
}

namespace OutfitSystem {
    extern SKSE::Trampoline* g_localTrampoline;
    extern SKSE::Trampoline* g_branchTrampoline;
    void ApplyPlayerSkinningHooks();
}// namespace OutfitSystem