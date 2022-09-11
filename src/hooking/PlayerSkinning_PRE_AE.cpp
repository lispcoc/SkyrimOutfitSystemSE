#ifdef SKYRIM_VERSION_IS_PRE_AE

#include "ArmorAddonOverrideService.h"

#include <xbyak/xbyak.h>

namespace OutfitSystem {
    SKSE::Trampoline* g_localTrampoline = nullptr;
    SKSE::Trampoline* g_branchTrampoline = nullptr;

    bool ShouldOverrideSkinning(RE::TESObjectREFR* target) {
        if (!target) return false;
        if (!ArmorAddonOverrideService::GetInstance().enabled) return false;
        auto actor = skyrim_cast<RE::Actor*>(target);
        if (!actor) return false;
        if (!ArmorAddonOverrideService::GetInstance().shouldOverride(actor)) return false;
        return true;
    }

    class EquippedArmorVisitor: public RE::InventoryChanges::IItemChangeVisitor {
        //
        // If the player has a shield equipped, and if we're not overriding that
        // shield, then we need to grab the equipped shield's worn-flags.
        //
    public:
        virtual ReturnType Visit(RE::InventoryEntryData* data) override {
            auto form = data->object;
            if (form && form->formType == RE::FormType::Armor) {
                equipped.emplace(skyrim_cast<RE::TESObjectARMO*>(form));
            }
            return ReturnType::kContinue; // Return true to "continue visiting".
        };

        std::unordered_set<RE::TESObjectARMO*> equipped;
    };

    REL::ID TESObjectARMO_ApplyArmorAddon(17392); // 0x00228AD0 in 1.5.73

    namespace DontVanillaSkinPlayer {
        bool _stdcall ShouldOverride(RE::TESObjectARMO* armor, RE::TESObjectREFR* target) {
            if (!ShouldOverrideSkinning(target))
                return false;
            auto& svc = ArmorAddonOverrideService::GetInstance();
            auto& outfit = svc.currentOutfit((RE::Actor*) target);
            auto actor = skyrim_cast<RE::Actor*>(target);
            if (!actor) {
                // Actor failed to cast...
                LOG(info, "ShouldOverride: Failed to cast target to Actor.");
                return true;
            }
            auto inventory = target->GetInventoryChanges();
            EquippedArmorVisitor visitor;
            if (inventory) {
                inventory->ExecuteVisitorOnWorn(&visitor);
            } else {
                LOG(info, "ShouldOverride: Unable to get target inventory.");
                return true;
            }
            // Block the item (return true) if the item isn't in the display set.
            return outfit.computeDisplaySet(visitor.equipped).count(armor) == 0;
        }

        REL::ID DontVanillaSkinPlayer_Hook_ID(24232);
        std::uintptr_t DontVanillaSkinPlayer_Hook(DontVanillaSkinPlayer_Hook_ID.address() + 0x302); // 0x00364652 in 1.5.73

        void Apply() {
            LOG(info, "Patching vanilla player skinning");
            LOG(info, "TESObjectARMO_ApplyArmorAddon = {:x}", TESObjectARMO_ApplyArmorAddon.address() - REL::Module::get().base());
            LOG(info, "DontVanillaSkinPlayer_Hook = {:x}", DontVanillaSkinPlayer_Hook - REL::Module::get().base());
            {
                struct DontVanillaSkinPlayer_Code: Xbyak::CodeGenerator {
                    DontVanillaSkinPlayer_Code() {
                        Xbyak::Label j_Out;
                        Xbyak::Label f_ApplyArmorAddon;
                        Xbyak::Label f_ShouldOverride;

                        // armor in rcx, target in r13
                        push(rcx);
                        push(rdx);
                        push(r9);
                        push(r8);
                        mov(rdx, r13);
                        sub(rsp, 0x40);
                        call(ptr[rip + f_ShouldOverride]);
                        add(rsp, 0x40);
                        pop(r8);
                        pop(r9);
                        pop(rdx);
                        pop(rcx);
                        test(al, al);
                        jnz(j_Out);
                        call(ptr[rip + f_ApplyArmorAddon]);
                        L(j_Out);
                        jmp(ptr[rip]);
                        dq(DontVanillaSkinPlayer_Hook + 0x5);

                        L(f_ApplyArmorAddon);
                        dq(TESObjectARMO_ApplyArmorAddon.address());

                        L(f_ShouldOverride);
                        dq(uintptr_t(ShouldOverride));
                    }
                };
                DontVanillaSkinPlayer_Code gen;
                void* code = g_localTrampoline->allocate(gen);

                LOG(info, "Patching vanilla player skinning at addr = {:x}. base = {:x}", DontVanillaSkinPlayer_Hook, REL::Module::get().base());
                g_branchTrampoline->write_branch<5>(DontVanillaSkinPlayer_Hook, code);
            }
            LOG(info, "Done");
        }
    } // namespace DontVanillaSkinPlayer

    namespace ShimWornFlags {
        std::uint32_t OverrideWornFlags(RE::InventoryChanges* inventory, RE::TESObjectREFR* target) {
            std::uint32_t mask = 0;
            auto actor = skyrim_cast<RE::Actor*>(target);
            if (!actor) return mask;
            auto& svc = ArmorAddonOverrideService::GetInstance();
            auto& outfit = svc.currentOutfit(actor);
            EquippedArmorVisitor visitor;
            inventory->ExecuteVisitorOnWorn(&visitor);
            auto displaySet = outfit.computeDisplaySet(visitor.equipped);
            for (auto& armor : displaySet) {
                mask |= static_cast<std::uint32_t>(armor->GetSlotMask());
            }
            return mask;
        }

        REL::ID ShimWornFlags_Hook_ID(24220);
        std::uintptr_t ShimWornFlags_Hook(ShimWornFlags_Hook_ID.address() + 0x7C); // 0x00362F0C in 1.5.73

        REL::ID InventoryChanges_GetWornMask(15806); // 0x001D9040 in 1.5.73

        void Apply() {
            LOG(info, "Patching shim worn flags");
            LOG(info, "ShimWornFlags_Hook = {:x}", ShimWornFlags_Hook - REL::Module::get().base());
            LOG(info, "InventoryChanges_GetWornMask = {:x}", InventoryChanges_GetWornMask.address() - REL::Module::get().base());
            {
                struct ShimWornFlags_Code: Xbyak::CodeGenerator {
                    ShimWornFlags_Code() {
                        Xbyak::Label j_SuppressVanilla;
                        Xbyak::Label j_Out;
                        Xbyak::Label f_ShouldOverrideSkinning;
                        Xbyak::Label f_GetWornMask;
                        Xbyak::Label f_OverrideWornFlags;

                        // target in rsi
                        push(rcx);
                        mov(rcx, rsi);
                        sub(rsp, 0x8); // Ensure 16-byte alignment of stack pointer
                        sub(rsp, 0x20);
                        call(ptr[rip + f_ShouldOverrideSkinning]);
                        add(rsp, 0x20);
                        add(rsp, 0x8);
                        pop(rcx);
                        test(al, al);
                        jnz(j_SuppressVanilla);
                        call(ptr[rip + f_GetWornMask]);
                        jmp(j_Out);

                        L(j_SuppressVanilla);
                        push(rdx);
                        mov(rdx, rsi);
                        sub(rsp, 0x8); // Ensure 16-byte alignment of stack pointer
                        sub(rsp, 0x20);
                        call(ptr[rip + f_OverrideWornFlags]);
                        add(rsp, 0x20);
                        add(rsp, 0x8);
                        pop(rdx);

                        L(j_Out);
                        jmp(ptr[rip]);
                        dq(ShimWornFlags_Hook + 0x5);

                        L(f_ShouldOverrideSkinning);
                        dq(uintptr_t(ShouldOverrideSkinning));

                        L(f_GetWornMask);
                        dq(InventoryChanges_GetWornMask.address());

                        L(f_OverrideWornFlags);
                        dq(uintptr_t(OverrideWornFlags));
                    }
                };
                ShimWornFlags_Code gen;
                void* code = g_localTrampoline->allocate(gen);

                LOG(info, "Patching shim worn flags at addr = {:x}. base = {:x}", ShimWornFlags_Hook, REL::Module::get().base());
                g_branchTrampoline->write_branch<5>(ShimWornFlags_Hook, code);
            }
            LOG(info, "Done");
        }
    } // namespace ShimWornFlags

    namespace CustomSkinPlayer {
        void Custom(RE::Actor* target, RE::ActorWeightModel* actorWeightModel) {
            if (!skyrim_cast<RE::Actor*>(target)) {
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
            auto& outfit = svc.currentOutfit((RE::Actor*) target);

            // Get actor inventory and equipped items
            auto inventory = target->GetInventoryChanges();
            EquippedArmorVisitor visitor;
            if (inventory) {
                inventory->ExecuteVisitorOnWorn(&visitor);
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
                    armor->ApplyArmorAddon(race, actorWeightModel, isFemale);
                }
            }
        }

        REL::ID CustomSkinPlayer_Hook_ID(24231);
        std::uintptr_t CustomSkinPlayer_Hook(CustomSkinPlayer_Hook_ID.address() + 0x81); // 0x00364301 in 1.5.73

        REL::ID InventoryChanges_ExecuteVisitorOnWorn(15856); // 0x001E51D0 in 1.5.73

        void Apply() {
            LOG(info, "Patching custom skin player");
            LOG(info, "CustomSkinPlayer_Hook = {:x}", CustomSkinPlayer_Hook - REL::Module::get().base());
            LOG(info, "InventoryChanges_ExecuteVisitorOnWorn = {:x}", InventoryChanges_ExecuteVisitorOnWorn.address() - REL::Module::get().base());
            {
                struct CustomSkinPlayer_Code: Xbyak::CodeGenerator {
                    CustomSkinPlayer_Code() {
                        Xbyak::Label j_Out;
                        Xbyak::Label f_Custom;
                        Xbyak::Label f_ExecuteVisitorOnWorn;
                        Xbyak::Label f_ShouldOverrideSkinning;

                        // call original function
                        call(ptr[rip + f_ExecuteVisitorOnWorn]);

                        push(rcx);
                        mov(rcx, rbx);
                        sub(rsp, 0x8); // Ensure 16-byte alignment of stack pointer
                        sub(rsp, 0x20);
                        call(ptr[rip + f_ShouldOverrideSkinning]);
                        add(rsp, 0x20);
                        add(rsp, 0x8);
                        pop(rcx);

                        test(al, al);
                        jz(j_Out);

                        push(rdx);
                        push(rcx);
                        mov(rcx, rbx);
                        mov(rdx, rdi);
                        sub(rsp, 0x20);
                        call(ptr[rip + f_Custom]);
                        add(rsp, 0x20);
                        pop(rcx);
                        pop(rdx);

                        L(j_Out);
                        jmp(ptr[rip]);
                        dq(CustomSkinPlayer_Hook + 0x5);

                        L(f_Custom);
                        dq(uintptr_t(Custom));

                        L(f_ExecuteVisitorOnWorn);
                        dq(InventoryChanges_ExecuteVisitorOnWorn.address());

                        L(f_ShouldOverrideSkinning);
                        dq(uintptr_t(ShouldOverrideSkinning));
                    }
                };
                CustomSkinPlayer_Code gen;
                void* code = g_localTrampoline->allocate(gen);

                LOG(info, "AVI: Patching custom skin player at addr = {}. base = {}", CustomSkinPlayer_Hook, REL::Module::get().base());
                g_branchTrampoline->write_branch<5>(CustomSkinPlayer_Hook, code);
            }
            LOG(info, "Done");
        }
    } // namespace CustomSkinPlayer

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
        class _Visitor: public RE::InventoryChanges::IItemChangeVisitor {
            //
            // Bethesda used a visitor to add armor-addons to the ActorWeightModel in the first
            // place (see call stack for DontVanillaSkinPlayer patch), so why not use a similar
            // visitor to check for conflicts?
            //
        public:
            virtual ReturnType Visit(RE::InventoryEntryData* data) override {
                // TODO: [SlotPassthru] We might be able to leave this as-is.
                auto form = data->object;
                if (form && form->formType == RE::FormType::Armor) {
                    auto armor = reinterpret_cast<RE::TESObjectARMO*>(form);
                    if (armor->TestBodyPartByIndex(this->conflictIndex)) {
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
                return ReturnType::kContinue; // True to continue visiting
            };

            RE::Actor* target;
            std::uint32_t     conflictIndex = 0;
        };
        void Inner(std::uint32_t bodyPartForNewItem, RE::Actor* target) {
            auto inventory = target->GetInventoryChanges();
            if (inventory) {
                _Visitor visitor;
                visitor.conflictIndex = bodyPartForNewItem;
                visitor.target = target;
                inventory->ExecuteVisitorOnWorn(&visitor);
            } else {
                LOG(info, "OverridePlayerSkinning: Conflict check failed: no inventory!");
            }
        }
        bool ShouldOverride(RE::TESForm* item) {
            //
            // We only hijack equipping for armors, so I'd like for this patch to only
            // apply to armors as well. It shouldn't really matter -- before I added
            // this check, weapons and the like tested in-game with no issues, likely
            // because they're handled very differently -- but I just wanna be sure.
            // We should use vanilla code whenever we don't need to NOT use it.
            //
            return (item->formType == RE::FormType::Armor);
        }

        REL::ID FixEquipConflictCheck_Hook_ID(36979);
        std::uintptr_t FixEquipConflictCheck_Hook(FixEquipConflictCheck_Hook_ID.address() + 0x97); // 0x0060CAC7 in 1.5.73

        REL::ID BGSBipedObjectForm_TestBodyPartByIndex(14026); // 0x001820A0 in 1.5.73

        void Apply() {
            LOG(info, "Patching fix for equip conflict check");
            LOG(info, "FixEquipConflictCheck_Hook = {:x}", FixEquipConflictCheck_Hook - REL::Module::get().base());
            LOG(info, "BGSBipedObjectForm_TestBodyPartByIndex = {:x}", BGSBipedObjectForm_TestBodyPartByIndex.address() - REL::Module::get().base());
            {
                struct FixEquipConflictCheck_Code: Xbyak::CodeGenerator {
                    FixEquipConflictCheck_Code() {
                        Xbyak::Label j_Out;
                        Xbyak::Label j_Exit;
                        Xbyak::Label f_Inner;
                        Xbyak::Label f_TestBodyPartByIndex;
                        Xbyak::Label f_ShouldOverride;

                        call(ptr[rip + f_TestBodyPartByIndex]);
                        test(al, al);
                        jz(j_Exit);

                        // rsp+0x10: item
                        // rdi: Actor
                        // rbx: Body Slot
                        push(rcx);
                        // Set RCX to the RDX argument of the patched function
                        // RSP + Argument offset rel to original entry + Offset from push above
                        // + Offset from pushes in original entry
                        mov(rcx, ptr[rsp + 0x10 + 0x08 + 0xC8]);
                        sub(rsp, 0x8); // Ensure 16-byte alignment of stack pointer
                        sub(rsp, 0x20);
                        call(ptr[rip + f_ShouldOverride]);
                        add(rsp, 0x20);
                        add(rsp, 0x8);
                        pop(rcx);
                        test(al, al);
                        mov(rax, 1);
                        jz(j_Out);
                        push(rcx);
                        push(rdx);
                        mov(rcx, rbx);
                        mov(rdx, rdi);
                        sub(rsp, 0x20);
                        call(ptr[rip + f_Inner]);
                        add(rsp, 0x20);
                        pop(rdx);
                        pop(rcx);

                        L(j_Exit);
                        xor_(al, al);

                        L(j_Out);
                        jmp(ptr[rip]);
                        dq(FixEquipConflictCheck_Hook + 0x5);

                        L(f_TestBodyPartByIndex);
                        dq(BGSBipedObjectForm_TestBodyPartByIndex.address());

                        L(f_ShouldOverride);
                        dq(uintptr_t(ShouldOverride));

                        L(f_Inner);
                        dq(uintptr_t(Inner));
                    }
                };
                FixEquipConflictCheck_Code gen;
                void* code = g_localTrampoline->allocate(gen);

                LOG(info, "AVI: Patching fix equip conflict check at addr = {:x}. base = {:x}", FixEquipConflictCheck_Hook, REL::Module::get().base());
                g_branchTrampoline->write_branch<5>(FixEquipConflictCheck_Hook, code);
            }
            LOG(info, "Done");
        }
    } // namespace FixEquipConflictCheck

    void ApplyPlayerSkinningHooks() {
        DontVanillaSkinPlayer::Apply();
        ShimWornFlags::Apply();
        CustomSkinPlayer::Apply();
        FixEquipConflictCheck::Apply();
    }
} // namespace OutfitSystem
#endif