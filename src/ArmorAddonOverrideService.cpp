#include "ArmorAddonOverrideService.h"

#include "RE/REAugments.h"

void _assertWrite(bool result, const char* err) {
    if (!result)
        throw save_error(err);
}
void _assertRead(bool result, const char* err) {
    if (!result)
        throw load_error(err);
}

namespace SlotPolicy {
    // Negative values mean "advanced option"
    std::array<Metadata, kNumModes> g_policiesMetadata = {
        Metadata{"XXXX", 100, true},// Never show anything
        Metadata{"XXXE", 101, true},// If outfit and equipped, show equipped
        Metadata{"XXXO", 2, false}, // If outfit and equipped, show outfit (require equipped, no passthrough)
        Metadata{"XXOX", 102, true},// If only outfit, show outfit
        Metadata{"XXOE", 103, true},// If only outfit, show outfit. If both, show equipped
        Metadata{"XXOO", 1, false}, // If outfit, show outfit (always show outfit, no passthough)
        Metadata{"XEXX", 104, true},// If only equipped, show equipped
        Metadata{"XEXE", 105, true},// If equipped, show equipped
        Metadata{"XEXO", 3, false}, // If only equipped, show equipped. If both, show outfit
        Metadata{"XEOX", 106, true},// If only equipped, show equipped. If only outfit, show outfit
        Metadata{"XEOE", 107, true},// If only equipped, show equipped. If only outfit, show outfit. If both, show equipped
        Metadata{"XEOO", 108, true} // If only equipped, show equipped. If only outfit, show outfit. If both, show outfit
    };

    Selection select(Mode policy, bool hasEquipped, bool hasOutfit) {
        if (policy >= Mode::kNumModes) {
            LOG(err, "Invalid slot preference {}", policy);
            policy = Mode::XXXX;
        }
        char out;
        if (!hasEquipped && !hasOutfit) {
            out = g_policiesMetadata[policy].code[0];
        } else if (hasEquipped && !hasOutfit) {
            out = g_policiesMetadata[policy].code[1];
        } else if (!hasEquipped && hasOutfit) {
            out = g_policiesMetadata[policy].code[2];
        } else if (hasEquipped && hasOutfit) {
            out = g_policiesMetadata[policy].code[3];
        }
        switch (out) {
        case 'E':
            return Selection::EQUIPPED;
        case 'O':
            return Selection::OUTFIT;
        default:
            return Selection::EMPTY;
        }
    }
}// namespace SlotPolicy


OutfitService& GetRustInstance() {
    static rust::Box<OutfitService> instance = outfit_service_create();
    return *instance;
}
