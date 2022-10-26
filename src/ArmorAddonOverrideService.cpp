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

OutfitService& GetRustInstance() {
    static rust::Box<OutfitService> instance = outfit_service_create();
    return *instance;
}
