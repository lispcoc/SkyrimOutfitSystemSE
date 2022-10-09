//
// Created by m on 10/8/2022.
//

#include "RE/REAugments.h"

void RE::InventoryChangesAugments::ExecuteVisitor(RE::InventoryChanges* thisPtr, RE::InventoryChanges::IItemChangeVisitor * a_visitor) {
    using func_t = decltype(&RE::InventoryChangesAugments::ExecuteVisitor);
    REL::Relocation<func_t> func{ REL::ID(15855) };
    return func(thisPtr, a_visitor);
}

void RE::InventoryChangesAugments::ExecuteAugmentVisitor(RE::InventoryChanges* thisPtr, RE::IItemChangeVisitorAugment * a_visitor) {
    using func_t = decltype(&RE::InventoryChangesAugments::ExecuteAugmentVisitor);
    REL::Relocation<func_t> func{ REL::ID(15855) };
    return func(thisPtr, a_visitor);
}


void RE::InventoryChangesAugments::ExecuteVisitorOnWorn(RE::InventoryChanges* thisPtr, RE::InventoryChanges::IItemChangeVisitor * a_visitor) {
    using func_t = decltype(&RE::InventoryChangesAugments::ExecuteVisitorOnWorn);
    REL::Relocation<func_t> func{ REL::ID(15856) };
    return func(thisPtr, a_visitor);
}

void RE::InventoryChangesAugments::ExecuteAugmentVisitorOnWorn(RE::InventoryChanges* thisPtr, RE::IItemChangeVisitorAugment * a_visitor) {
    using func_t = decltype(&RE::InventoryChangesAugments::ExecuteAugmentVisitorOnWorn);
    REL::Relocation<func_t> func{ REL::ID(15856) };
    return func(thisPtr, a_visitor);
}

void RE::AIProcessAugments::SetEquipFlag(RE::AIProcess* thisPtr, RE::AIProcessAugments::Flag a_flag) {
    using func_t = decltype(&RE::AIProcessAugments::SetEquipFlag);
    REL::Relocation<func_t> func{ REL::ID(38867) };
    return func(thisPtr, a_flag);}

void RE::AIProcessAugments::UpdateEquipment(RE::AIProcess* thisPtr, RE::Actor* a_actor) {
    using func_t = decltype(&RE::AIProcessAugments::UpdateEquipment);
    REL::Relocation<func_t> func{ REL::ID(38404) };
    return func(thisPtr, a_actor);
}

bool RE::TESObjectARMOAugments::ApplyArmorAddon(RE::TESObjectARMO* thisPtr, RE::TESRace* a_race, RE::ActorWeightModel* a_model, bool a_isFemale) {
    using func_t = decltype(&RE::TESObjectARMOAugments::ApplyArmorAddon);
    REL::Relocation<func_t> func{ REL::ID(17392) };
    return func(thisPtr, a_race, a_model, a_isFemale);}

bool RE::TESObjectARMOAugments::TestBodyPartByIndex(RE::TESObjectARMO* thisPtr, std::uint32_t a_index) {
    using func_t = decltype(&RE::TESObjectARMOAugments::TestBodyPartByIndex);
    REL::Relocation<func_t> func{ REL::ID(17395) };
    return func(thisPtr, a_index);
}
