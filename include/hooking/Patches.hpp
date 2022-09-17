//
// Created by m on 9/11/2022.
//

#ifndef SKYRIMOUTFITSYSTEMSE_INCLUDE_HOOKING_PATCHES_HPP
#define SKYRIMOUTFITSYSTEMSE_INCLUDE_HOOKING_PATCHES_HPP

namespace Hooking {
    bool ShouldOverrideSkinning(RE::TESObjectREFR* target);

    namespace DontVanillaSkinPlayer {
        bool ShouldOverride(RE::TESObjectARMO* armor, RE::TESObjectREFR* target);
    }

    namespace ShimWornFlags {
        std::uint32_t OverrideWornFlags(RE::InventoryChanges* inventory, RE::TESObjectREFR* target);
    }

    namespace CustomSkinPlayer {
        void Custom(RE::Actor* target, RE::ActorWeightModel* actorWeightModel);
    }

    namespace FixEquipConflictCheck {
        void Inner(std::uint32_t bodyPartForNewItem, RE::Actor* target);
        bool ShouldOverride(RE::TESForm* item);
    }// namespace FixEquipConflictCheck

    namespace FixSkillLeveling {
        struct Visitor;
        bool Inner(RE::BipedAnim* biped, Visitor* bipedVisitor);
    }// namespace FixSkillLeveling

    namespace RTTIPrinter {
        void Print_RTTI(RE::InventoryChanges::IItemChangeVisitor* target);
    }
}// namespace Hooking

#endif//SKYRIMOUTFITSYSTEMSE_INCLUDE_HOOKING_PATCHES_HPP
