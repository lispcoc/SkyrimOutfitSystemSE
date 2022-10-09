//
// Created by m on 9/11/2022.
//

#include "hooking/Patches.hpp"

#include <bit>

#include "ArmorAddonOverrideService.h"
#include "Utility.h"

namespace Hooking {
    SKSE::Trampoline* g_localTrampoline = nullptr;
    SKSE::Trampoline* g_branchTrampoline = nullptr;

    bool ShouldOverrideSkinning(RE::TESObjectREFR* target) {
        LogExit exitPrint("ShouldOverrideSkinning"sv);
        if (!target) {
            LOG(warn, "Target was null");
            return false;
        }
        if (!ArmorAddonOverrideService::GetInstance().enabled) return false;
        auto actor = skyrim_cast<RE::Actor*>(target);
        if (!actor) {
            LOG(warn, "Target failed to cast to RE::Actor");
            return false;
        }
        if (!ArmorAddonOverrideService::GetInstance().shouldOverride(actor->GetHandle().native_handle())) return false;
        return true;
    }

    class EquippedArmorVisitor: public RE::IItemChangeVisitorAugment {
        //
        // If the player has a shield equipped, and if we're not overriding that
        // shield, then we need to grab the equipped shield's worn-flags.
        //
    public:
        virtual VisitorReturn Visit(RE::InventoryEntryData* data) override {
            auto form = data->object;
            if (form && form->formType == RE::FormType::Armor) {
                equipped.emplace(skyrim_cast<RE::TESObjectARMO*>(form));
            }
            return VisitorReturn::kContinue;// Return true to "continue visiting".
        };

        std::unordered_set<RE::TESObjectARMO*> equipped;
    };

    template<typename F>
    class EquippedVisitorFn: public RE::IItemChangeVisitorAugment {
        //
        // If the player has a shield equipped, and if we're not overriding that
        // shield, then we need to grab the equipped shield's worn-flags.
        //
    public:
        explicit EquippedVisitorFn(F callable) : callable(callable){};
        virtual VisitorReturn Visit(RE::InventoryEntryData* data) override {
            auto form = data->object;
            if (form) {
                callable(data->object);
            }
            return VisitorReturn::kContinue;// Return true to "continue visiting".
        };

        F callable;
    };

    namespace DontVanillaSkinPlayer {
        bool ShouldOverride(RE::TESObjectARMO* armor, RE::TESObjectREFR* target) {
            LogExit exitPrint("DontVanillaSkinPlayer.ShouldOverride"sv);
            if (!ShouldOverrideSkinning(target)) { return false; }
            auto& svc = ArmorAddonOverrideService::GetInstance();
            auto actor = skyrim_cast<RE::Actor*>(target);
            if (!actor) {
                // Actor failed to cast...
                LOG(warn, "ShouldOverride: Failed to cast target to Actor.");
                return true;
            }
            auto& outfit = svc.currentOutfit(actor->GetHandle().native_handle());
            auto inventory = target->GetInventoryChanges();
            EquippedArmorVisitor visitor;
            if (inventory) {
                RE::InventoryChangesAugments::ExecuteAugmentVisitorOnWorn(inventory, &visitor);
            } else {
                LOG(warn, "ShouldOverride: Unable to get target inventory.");
                return true;
            }
            // Block the item (return true) if the item isn't in the display set.
            return outfit.computeDisplaySet(visitor.equipped).count(armor) == 0;
        }
    }// namespace DontVanillaSkinPlayer

    namespace ShimWornFlags {
        std::uint32_t OverrideWornFlags(RE::InventoryChanges* inventory, RE::TESObjectREFR* target) {
            LogExit exitPrint("ShimWornFlags.OverrideWornFlags"sv);
            std::uint32_t mask = 0;
            auto actor = skyrim_cast<RE::Actor*>(target);
            if (!actor) return mask;
            auto& svc = ArmorAddonOverrideService::GetInstance();
            auto& outfit = svc.currentOutfit(actor->GetHandle().native_handle());
            EquippedArmorVisitor visitor;
            RE::InventoryChangesAugments::ExecuteAugmentVisitorOnWorn(inventory, &visitor);
            auto displaySet = outfit.computeDisplaySet(visitor.equipped);
            for (auto& armor : displaySet) {
                mask |= static_cast<std::uint32_t>(armor->GetSlotMask());
            }
            return mask;
        }
    }// namespace ShimWornFlags

    namespace CustomSkinPlayer {
        void Custom(RE::Actor* target, RE::ActorWeightModel* actorWeightModel) {
            LogExit exitPrint("CustomSkinPlayer.Custom"sv);
            auto actor = skyrim_cast<RE::Actor*>(target);
            if (!actor) {
                // Actor failed to cast...
                LOG(info, "Custom: Failed to cast target to Actor.");
                return;
            }
            // Get basic actor information (race and sex)
            if (!actorWeightModel)
                return;
            auto base = skyrim_cast<RE::TESNPC*>(target->data.objectReference);
            if (!base)
                return;
            auto race = base->race;
            bool isFemale = base->IsFemale();
            //
            auto& svc = ArmorAddonOverrideService::GetInstance();
            auto& outfit = svc.currentOutfit(actor->GetHandle().native_handle());

            // Get actor inventory and equipped items
            auto inventory = target->GetInventoryChanges();
            EquippedArmorVisitor visitor;
            if (inventory) {
                RE::InventoryChangesAugments::ExecuteAugmentVisitorOnWorn(inventory, &visitor);
            } else {
                LOG(info, "Custom: Unable to get target inventory.");
                return;
            }

            // Compute the display set.
            auto displaySet = outfit.computeDisplaySet(visitor.equipped);

            // Compute the remaining items to be applied to the player
            // We assume that the DontVanillaSkinPlayer already passed through
            // the equipped items that will be shown, so we only need to worry about the
            // items that are not equipped but which are in the outfit or which are masked
            // by the outfit.
            std::unordered_set<RE::TESObjectARMO*> applySet;
            for (const auto& item : displaySet) {
                if (visitor.equipped.find(item) == visitor.equipped.end()) applySet.insert(item);
            }

            for (auto it = applySet.cbegin(); it != applySet.cend(); ++it) {
                // TODO: [SlotPassthru] Also do the same iteration over the passthrough equipped items?
                RE::TESObjectARMO* armor = *it;
                if (armor) {
                    RE::TESObjectARMOAugments::ApplyArmorAddon(armor, race, actorWeightModel, isFemale);
                }
            }
        }
    }// namespace CustomSkinPlayer

    namespace FixEquipConflictCheck {
        //
        // When you try to equip an item, the game loops over the armors in your ActorWeightModel
        // rather than your other worn items. Because we're tampering with what goes into the AWM,
        // this means that conflict checks are run against your outfit instead of your equipment,
        // unless we patch in a fix. (For example, if your outfit doesn't include a helmet, then
        // you'd be able to stack helmets endlessly without this patch.)
        //
        // The loop in question is performed in Actor::Unk_120, which is also generally responsible
        // for equipping items at all.
        //
        class _Visitor: public RE::IItemChangeVisitorAugment {
            //
            // Bethesda used a visitor to add armor-addons to the ActorWeightModel in the first
            // place (see call stack for DontVanillaSkinPlayer patch), so why not use a similar
            // visitor to check for conflicts?
            //
        public:
            virtual VisitorReturn Visit(RE::InventoryEntryData* data) override {
                // TODO: [SlotPassthru] We might be able to leave this as-is.
                auto form = data->object;
                if (form && form->formType == RE::FormType::Armor) {
                    auto armor = skyrim_cast<RE::TESObjectARMO*>(form);
                    if (armor && RE::TESObjectARMOAugments::TestBodyPartByIndex(armor, this->conflictIndex)) {
                        auto em = RE::ActorEquipManager::GetSingleton();
                        //
                        // TODO: The third argument to this call is meant to be a
                        // BaseExtraList*, and Bethesda supplies one when calling from Unk_120.
                        // Can we get away with a nullptr here, or do we have to find the
                        // BaseExtraList that contains an ExtraWorn?
                        //
                        // I'm not sure how to investigate this, but I did run one test, and
                        // that works properly: I gave myself ten Falmer Helmets and applied
                        // different enchantments to two of them (leaving the others
                        // unenchanted). In tests, I was unable to stack the helmets with each
                        // other or with other helmets, suggesting that the BaseExtraList may
                        // not be strictly necessary.
                        //
                        em->UnequipObject(this->target, form, nullptr, 1, nullptr, false, false, true, false, nullptr);
                    }
                }
                return VisitorReturn::kContinue;// True to continue visiting
            };

            RE::Actor* target;
            std::uint32_t conflictIndex = 0;
        };
        void Inner(std::uint32_t bodyPartForNewItem, RE::Actor* target) {
            LogExit exitPrint("FixEquipConflictCheck.Inner"sv);
            auto inventory = target->GetInventoryChanges();
            if (inventory) {
                _Visitor visitor;
                visitor.conflictIndex = bodyPartForNewItem;
                visitor.target = target;
                RE::InventoryChangesAugments::ExecuteAugmentVisitorOnWorn(inventory, &visitor);
            } else {
                LOG(info, "OverridePlayerSkinning: Conflict check failed: no inventory!");
            }
        }
        bool ShouldOverride(RE::TESForm* item) {
            LogExit exitPrint("FixEquipConflictCheck.ShouldOverride"sv);
            //
            // We only hijack equipping for armors, so I'd like for this patch to only
            // apply to armors as well. It shouldn't really matter -- before I added
            // this check, weapons and the like tested in-game with no issues, likely
            // because they're handled very differently -- but I just wanna be sure.
            // We should use vanilla code whenever we don't need to NOT use it.
            //
            return (item->formType == RE::FormType::Armor);
        }
    }// namespace FixEquipConflictCheck

    namespace FixSkillLeveling {
        struct Visitor {
            RE::TESObjectARMO** shield;
            RE::TESObjectARMO** torso;
            std::uint32_t light;
            std::uint32_t heavy;
        };
        static_assert(sizeof(Visitor) == 0x18);

        bool Inner(RE::BipedAnim* biped, Visitor* bipedVisitor) {
            LogExit exitPrint("FixSkillLeveling.Inner"sv);
            auto target = biped->actorRef.get();// Retain via smart pointer.
            if (!target) return false;
            auto actor = skyrim_cast<RE::Actor*>(target.get());
            if (!actor) return false;
            if (!ShouldOverrideSkinning(actor)) return false;
            auto inventory = target->GetInventoryChanges();
            if (!inventory) return false;
            EquippedVisitorFn inventoryVisitor([&](RE::TESBoundObject* form) {
                if (form->formType == RE::FormType::Armor) {
                    auto armor = skyrim_cast<RE::TESObjectARMO*>(form);
                    if (!armor) return;
                    RE::BIPED_MODEL::ArmorType armorType = armor->GetArmorType();
                    auto mask = static_cast<std::uint32_t>(armor->GetSlotMask());
                    // Exclude shields.
                    if (armor->IsShield()) return;
                    // Count the number of slots taken by the armor. Body slot counts double.
                    int count = 0;
                    if (mask & static_cast<std::uint32_t>(RE::BIPED_MODEL::BipedObjectSlot::kBody)) count++;
                    count += std::popcount(mask);
                    if (armorType == RE::BIPED_MODEL::ArmorType::kLightArmor) bipedVisitor->light += count;
                    if (armorType == RE::BIPED_MODEL::ArmorType::kHeavyArmor) bipedVisitor->heavy += count;
                }
                return;
            });
            RE::InventoryChangesAugments::ExecuteAugmentVisitorOnWorn(inventory, &inventoryVisitor);
            return true;
        }
    }// namespace FixSkillLeveling

    namespace RTTIPrinter {
        void Print_RTTI(RE::InventoryChanges::IItemChangeVisitor* target) {
            LogExit exitPrint("RTTIPrinter.Print_RTTI"sv);
            void* object = (void*) target;
            void* vtable = *(void**) object;
            void* info_block = ((void**) vtable)[-1];
            uintptr_t id = ((unsigned long*) info_block)[3];
            uintptr_t info = id + REL::Module::get().base();
            char* name = (char*) info + 16;
            LOG(info, "vtable = {}, typeinfo = {}, typename = {}", vtable, info_block,
                name);
        }
    }// namespace RTTIPrinter
}// namespace Hooking