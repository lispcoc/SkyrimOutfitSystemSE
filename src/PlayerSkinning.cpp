#define XBYAK_NO_OP_NAMES 1
#include <xbyak/xbyak.h>

#include "RE/Inventory/InventoryChanges.h"
#include "RE/Inventory/InventoryEntryData.h"
#include "RE/FormComponents/TESForm/TESObjectREFR/Actor/Character/PlayerCharacter.h"
#include "RE/FormComponents/TESForm/TESObject/TESBoundObject/TESObjectARMO.h"
#include "RE/FormComponents/TESForm/TESObjectREFR/TESObjectREFR.h"
#include "RE/FormComponents/TESForm/TESForm.h"

#include "ArmorAddonOverrideService.h"
#include "skse64_common/Relocation.h"
#include "skse64_common/BranchTrampoline.h"
#include "skse64/GameRTTI.h"
#include "RE/Inventory/ActorEquipManager.h"

namespace OutfitSystem
{
    bool ShouldOverrideSkinning(RE::TESObjectREFR * target)
    {
        if (!ArmorAddonOverrideService::GetInstance().shouldOverride())
            return false;
        return target == RE::PlayerCharacter::GetSingleton();
    }

    const std::unordered_set<RE::TESObjectARMO*>& GetOverrideArmors()
    {
        auto& svc = ArmorAddonOverrideService::GetInstance();
        return svc.currentOutfit().armors;
    }

    class EquippedArmorVisitor : public RE::InventoryChanges::IItemChangeVisitor {
        //
        // If the player has a shield equipped, and if we're not overriding that
        // shield, then we need to grab the equipped shield's worn-flags.
        //
    public:
        virtual ReturnType Visit(RE::InventoryEntryData* data) override {
            auto form = data->object;
            if (form && form->formType == RE::FormType::Armor) {
                auto armor = reinterpret_cast<RE::TESObjectARMO*>(form);
                equipped.emplace(armor);
            }
            return ReturnType::kContinue; // Return true to "continue visiting".
        };

        std::unordered_set<RE::TESObjectARMO*> equipped;
    };

    REL::ID TESObjectARMO_ApplyArmorAddon(17392); // 0x00228AD0 in 1.5.73

    namespace DontVanillaSkinPlayer
    {
        bool _stdcall ShouldOverride(RE::TESObjectARMO* armor, RE::TESObjectREFR* target) {
            if (!ShouldOverrideSkinning(target)) return false;
            auto& svc = ArmorAddonOverrideService::GetInstance();
            auto& outfit = svc.currentOutfit();
            auto actor = reinterpret_cast<RE::Actor*>(Runtime_DynamicCast(static_cast<RE::TESObjectREFR*>(target), RTTI_TESObjectREFR, RTTI_Actor));
            if (!actor) {
                // Actor failed to cast...
                _MESSAGE("ShouldOverride: Failed to cast target to Actor.");
                return true;
            }
            auto inventory = target->GetInventoryChanges();
            EquippedArmorVisitor visitor;
            if (inventory) {
                inventory->ExecuteVisitorOnWorn(&visitor);
            } else {
                _MESSAGE("ShouldOverride: Unable to get target inventory.");
                return true;
            }
            // Block the item (return true) if the item isn't in the display set.
            return outfit.computeDisplaySet(visitor.equipped).count(armor) == 0;
        }

        REL::ID DontVanillaSkinPlayer_Hook_ID(24232);
        std::uintptr_t DontVanillaSkinPlayer_Hook(DontVanillaSkinPlayer_Hook_ID.address() + 0x302); // 0x00364652 in 1.5.73

        void Apply()
        {
            _MESSAGE("Patching vanilla player skinning");
            _MESSAGE("TESObjectARMO_ApplyArmorAddon = %p", TESObjectARMO_ApplyArmorAddon.address() - RelocationManager::s_baseAddr);
            _MESSAGE("DontVanillaSkinPlayer_Hook = %p", DontVanillaSkinPlayer_Hook - RelocationManager::s_baseAddr);
            {
                struct DontVanillaSkinPlayer_Code : Xbyak::CodeGenerator
                {
                    DontVanillaSkinPlayer_Code(void * buf) : CodeGenerator(4096, buf)
                    {
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
                        call(ptr[rip+f_ShouldOverride]);
                        add(rsp, 0x40);
                        pop(r8);
                        pop(r9);
                        pop(rdx);
                        pop(rcx);
                        test(al, al);
                        jnz(j_Out);
                        call(ptr[rip+f_ApplyArmorAddon]);
                        L(j_Out);
                        jmp(ptr[rip]);
                        dq(DontVanillaSkinPlayer_Hook + 0x5);

                        L(f_ApplyArmorAddon);
                        dq(TESObjectARMO_ApplyArmorAddon.address());

                        L(f_ShouldOverride);
                        dq(uintptr_t(ShouldOverride));
                    }
                };

                void* codeBuf = g_localTrampoline.StartAlloc();
                DontVanillaSkinPlayer_Code code(codeBuf);
                g_localTrampoline.EndAlloc(code.getCurr());

				_MESSAGE("AVI: Patching vanilla player skinning at addr = %llX. base = %llX", DontVanillaSkinPlayer_Hook, RelocationManager::s_baseAddr);
                g_branchTrampoline.Write5Branch(DontVanillaSkinPlayer_Hook,
                    uintptr_t(code.getCode()));
            }
            _MESSAGE("Done");
        }
    }

    namespace ShimWornFlags
    {
        UInt32 OverrideWornFlags(RE::InventoryChanges * inventory) {
            UInt32 mask = 0;
            //
            auto& svc = ArmorAddonOverrideService::GetInstance();
            auto& outfit = svc.currentOutfit();
            EquippedArmorVisitor visitor;
            inventory->ExecuteVisitorOnWorn(&visitor);
            auto displaySet = outfit.computeDisplaySet(visitor.equipped);
            for (auto& armor : displaySet) {
                mask |= static_cast<UInt32>(armor->GetSlotMask());
            }
            return mask;
        }

        REL::ID ShimWornFlags_Hook_ID(24220);
        std::uintptr_t ShimWornFlags_Hook(ShimWornFlags_Hook_ID.address() + 0x7C); // 0x00362F0C in 1.5.73

        REL::ID InventoryChanges_GetWornMask(15806); // 0x001D9040 in 1.5.73

        void Apply()
        {
            _MESSAGE("Patching shim worn flags");
            _MESSAGE("ShimWornFlags_Hook = %p", ShimWornFlags_Hook - RelocationManager::s_baseAddr);
            _MESSAGE("InventoryChanges_GetWornMask = %p", InventoryChanges_GetWornMask.address() - RelocationManager::s_baseAddr);
            {
                struct ShimWornFlags_Code : Xbyak::CodeGenerator
                {
                    ShimWornFlags_Code(void * buf) : CodeGenerator(4096, buf)
                    {
                        Xbyak::Label j_SuppressVanilla;
                        Xbyak::Label j_Out;
                        Xbyak::Label f_ShouldOverrideSkinning;
                        Xbyak::Label f_GetWornMask;
                        Xbyak::Label f_OverrideWornFlags;

                        // target in rsi
                        push(rcx);
                        mov(rcx, rsi);
                        sub(rsp, 0x20);
                        call(ptr[rip + f_ShouldOverrideSkinning]);
                        add(rsp, 0x20);
                        pop(rcx);
                        test(al, al);
                        jnz(j_SuppressVanilla);
                        call(ptr[rip + f_GetWornMask]);
                        jmp(j_Out);

                        L(j_SuppressVanilla);
                        call(ptr[rip + f_OverrideWornFlags]);

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

                void* codeBuf = g_localTrampoline.StartAlloc();
                ShimWornFlags_Code code(codeBuf);
                g_localTrampoline.EndAlloc(code.getCurr());

                g_branchTrampoline.Write5Branch(ShimWornFlags_Hook,
                    uintptr_t(code.getCode()));
            }
            _MESSAGE("Done");
        }
    }

    namespace CustomSkinPlayer
    {
        void Custom(RE::Actor* target, RE::ActorWeightModel * actorWeightModel) {
            // Get basic actor information (race and sex)
            if (!actorWeightModel)
                return;
            auto base = reinterpret_cast<RE::TESNPC*>(Runtime_DynamicCast(static_cast<RE::TESForm*>(target->data.objectReference), RTTI_TESForm, RTTI_TESNPC));
            if (!base)
                return;
            auto race = base->race;
            bool isFemale = base->IsFemale();
            //
            auto& svc = ArmorAddonOverrideService::GetInstance();
            auto& outfit = svc.currentOutfit();

            // Get actor inventory and equipped items
            auto actor = reinterpret_cast<RE::Actor*>(Runtime_DynamicCast(static_cast<RE::TESObjectREFR*>(target), RTTI_TESObjectREFR, RTTI_Actor));
            if (!actor) {
                // Actor failed to cast...
                _MESSAGE("Custom: Failed to cast target to Actor.");
                return;
            }
            auto inventory = target->GetInventoryChanges();
            EquippedArmorVisitor visitor;
            if (inventory) {
                inventory->ExecuteVisitorOnWorn(&visitor);
            } else {
                _MESSAGE("Custom: Unable to get target inventory.");
                return;
            }

            // Compute the display set.
            auto displaySet = outfit.computeDisplaySet(visitor.equipped);

            // Compute the remaining items to be applied to the player
            // We assume that the DontVanillaSkinPlayer already passed through
            // the equipped items that will be shown, so we only need to worry about the items that
            // are not equipped but which are in the outfit or which are masked by the outfit.
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

        void Apply()
        {
            _MESSAGE("Patching custom skin player");
            _MESSAGE("CustomSkinPlayer_Hook = %p", CustomSkinPlayer_Hook - RelocationManager::s_baseAddr);
            _MESSAGE("InventoryChanges_ExecuteVisitorOnWorn = %p", InventoryChanges_ExecuteVisitorOnWorn.address() - RelocationManager::s_baseAddr);
            {
                struct CustomSkinPlayer_Code : Xbyak::CodeGenerator
                {
                    CustomSkinPlayer_Code(void * buf) : CodeGenerator(4096, buf)
                    {
                        Xbyak::Label j_Out;
                        Xbyak::Label f_Custom;
                        Xbyak::Label f_ExecuteVisitorOnWorn;
                        Xbyak::Label f_ShouldOverrideSkinning;

                        // call original function
                        call(ptr[rip + f_ExecuteVisitorOnWorn]);

                        push(rcx);
                        mov(rcx, rbx);
                        sub(rsp, 0x20);
                        call(ptr[rip + f_ShouldOverrideSkinning]);
                        add(rsp, 0x20);
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

                void* codeBuf = g_localTrampoline.StartAlloc();
                CustomSkinPlayer_Code code(codeBuf);
                g_localTrampoline.EndAlloc(code.getCurr());

                g_branchTrampoline.Write5Branch(CustomSkinPlayer_Hook,
                    uintptr_t(code.getCode()));
            }
            _MESSAGE("Done");
        }
    }

    REL::ID FixEquipConflictCheck_Hook_ID(36979);
    std::uintptr_t FixEquipConflictCheck_Hook(FixEquipConflictCheck_Hook_ID.address() + 0x97); // 0x0060CAC7 in 1.5.73

    REL::ID BGSBipedObjectForm_TestBodyPartByIndex(14026); // 0x001820A0 in 1.5.73

    namespace FixEquipConflictCheck
    {
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
        class _Visitor : public RE::InventoryChanges::IItemChangeVisitor {
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
                        // TODO: The third argument to this call is meant to be a BaseExtraList*, 
                        // and Bethesda supplies one when calling from Unk_120. Can we get away 
                        // with a nullptr here, or do we have to find the BaseExtraList that 
                        // contains an ExtraWorn?
                        //
                        // I'm not sure how to investigate this, but I did run one test, and that 
                        // works properly: I gave myself ten Falmer Helmets and applied different 
                        // enchantments to two of them (leaving the others unenchanted). In tests, 
                        // I was unable to stack the helmets with each other or with other helmets, 
                        // suggesting that the BaseExtraList may not be strictly necessary.
                        //
                        em->UnequipObject(this->target, form, nullptr, 1, nullptr, false, false, true, false, nullptr);
                    }
                }
                return ReturnType::kContinue; // True to continue visiting
            };

            RE::Actor* target;
            UInt32     conflictIndex = 0;
        };
        void Inner(UInt32 bodyPartForNewItem, RE::Actor* target) {
            auto inventory = target->GetInventoryChanges();
            if (inventory) {
                _Visitor visitor;
                visitor.conflictIndex = bodyPartForNewItem;
                visitor.target = target;
                inventory->ExecuteVisitorOnWorn(&visitor);
            }
            else {
                _MESSAGE("OverridePlayerSkinning: Conflict check failed: no inventory!");
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
        void Apply()
        {
            _MESSAGE("Patching fix for equip conflict check");
            _MESSAGE("FixEquipConflictCheck_Hook = %p", FixEquipConflictCheck_Hook - RelocationManager::s_baseAddr);
            _MESSAGE("BGSBipedObjectForm_TestBodyPartByIndex = %p", BGSBipedObjectForm_TestBodyPartByIndex.address() - RelocationManager::s_baseAddr);
            {
                struct FixEquipConflictCheck_Code : Xbyak::CodeGenerator
                {
                    FixEquipConflictCheck_Code(void* buf) : CodeGenerator(4096, buf)
                    {
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
                        mov(rcx, ptr[rsp + 0x18]);
                        sub(rsp, 0x20);
                        call(ptr[rip + f_ShouldOverride]);
                        add(rsp, 0x20);
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

                void* codeBuf = g_localTrampoline.StartAlloc();
                FixEquipConflictCheck_Code code(codeBuf);
                g_localTrampoline.EndAlloc(code.getCurr());

                g_branchTrampoline.Write5Branch(FixEquipConflictCheck_Hook,
                                                uintptr_t(code.getCode()));
            }
            _MESSAGE("Done");
        }
    }

    void ApplyPlayerSkinningHooks()
    {
        DontVanillaSkinPlayer::Apply();
        ShimWornFlags::Apply();
        CustomSkinPlayer::Apply();
        FixEquipConflictCheck::Apply();
    }
}
