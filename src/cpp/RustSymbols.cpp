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
        auto a = ((RE::TESForm*) nullptr)->GetLocalFormID();
    }
    FormID TESDataHandler_LookupFormIDRawC(TESDataHandler* dh, FormID raw_form_id, const char* mod_name) {
        return dh->LookupFormIDRaw(raw_form_id, std::string_view(mod_name));
    }
    FormID TESDataHandler_LookupFormIDC(TESDataHandler* dh, FormID local_form_id, const char* mod_name) {
        return dh->LookupFormID(local_form_id, std::string_view(mod_name));
    }
}

