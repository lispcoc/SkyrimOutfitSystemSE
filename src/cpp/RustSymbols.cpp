//
// Created by m on 10/23/2022.
//

namespace RE {
    [[maybe_unused]] TESObjectARMO* ResolveARMOFormID(FormID id) {
        return skyrim_cast<RE::TESObjectARMO*>(RE::TESForm::LookupByID(id));
    }
    [[maybe_unused]] void instantiate() {
        RE::PlayerCharacter::GetSingleton();
        REL::Module::reset();
    }
    FormID TESDataHandler_LookupFormIDRawC(TESDataHandler* dh, FormID raw_form_id, const char* mod_name) {
        return dh->LookupFormIDRaw(raw_form_id, std::string_view(mod_name));
    }
}

