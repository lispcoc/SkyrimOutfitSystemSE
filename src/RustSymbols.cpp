//
// Created by m on 10/23/2022.
//

namespace RE {
    TESObjectARMO* ResolveARMOFormID(FormID id) {
        return skyrim_cast<RE::TESObjectARMO*>(RE::TESForm::LookupByID(id));
    }
}