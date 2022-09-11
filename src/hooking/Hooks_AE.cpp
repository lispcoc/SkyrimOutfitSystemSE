//
// Created by m on 9/11/2022.
//
#ifdef SKYRIM_VERSION_IS_AE

#include "hooking/Hooks.hpp"

#include <xbyak/xbyak.h>

#include "hooking/Patches.hpp"

namespace Hooking {
    SKSE::Trampoline* g_localTrampoline = nullptr;
    SKSE::Trampoline* g_branchTrampoline = nullptr;

    namespace DontVanillaSkinPlayer {
        REL::ID DontVanillaSkinPlayer_Hook_ID(24736);
        std::uintptr_t DontVanillaSkinPlayer_Hook(DontVanillaSkinPlayer_Hook_ID.address() + 0x302);// 0x00364652 in 1.5.73

        REL::ID TESObjectARMO_ApplyArmorAddon(17792);// 0x00228AD0 in 1.5.73

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
    }// namespace DontVanillaSkinPlayer

    namespace ShimWornFlags {
        REL::ID ShimWornFlags_Hook_ID(24724);
        std::uintptr_t ShimWornFlags_Hook(ShimWornFlags_Hook_ID.address() + 0x80);// 0x00362F0C in 1.5.73

        REL::ID InventoryChanges_GetWornMask(16044);// 0x001D9040 in 1.5.73

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

                        // target in rbx
                        push(rcx);
                        mov(rcx, rbx);
                        sub(rsp, 0x8);// Ensure 16-byte alignment of stack pointer
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
                        mov(rdx, rbx);
                        sub(rsp, 0x8);// Ensure 16-byte alignment of stack pointer
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
    }// namespace ShimWornFlags

    namespace CustomSkinPlayer {
        // This hook is completely different from pre-AE.
        // The function we wanted to patch (AE 24735 + 0x81) was inlined into AE 24725.
        // We might consider hooking both?
        REL::ID CustomSkinPlayer_Hook_ID(24725);
        std::uintptr_t CustomSkinPlayer_Hook(CustomSkinPlayer_Hook_ID.address() + 0x1EF);// 0x00364301 in 1.5.73

        REL::ID InventoryChanges_ExecuteVisitorOnWorn(16096);// 0x001E51D0 in 1.5.73

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
                        sub(rsp, 0x8);// Ensure 16-byte alignment of stack pointer
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
                        mov(rdx, r15);
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
    }// namespace CustomSkinPlayer

    namespace FixEquipConflictCheck {
        REL::ID FixEquipConflictCheck_Hook_ID(38004);
        std::uintptr_t FixEquipConflictCheck_Hook(FixEquipConflictCheck_Hook_ID.address() + 0x97);// 0x0060CAC7 in 1.5.73

        REL::ID BGSBipedObjectForm_TestBodyPartByIndex(14119);// 0x001820A0 in 1.5.73

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
                        sub(rsp, 0x8);// Ensure 16-byte alignment of stack pointer
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
    }// namespace FixEquipConflictCheck

    namespace RTTIPrinter {
        REL::ID InventoryChanges_ExecuteVisitorOnWorn(16096);// 0x001E51D0 in 1.5.73
        std::uintptr_t InventoryChanges_ExecuteVisitorOnWorn_Hook(
            InventoryChanges_ExecuteVisitorOnWorn.address() + 0x2A);// 0x00364301 in 1.5.73

        void Apply() {
            LOG(info, "Patching RTTI print");
            LOG(info, "InventoryChanges_ExecuteVisitorOnWorn_Hook = {:x}",
                InventoryChanges_ExecuteVisitorOnWorn_Hook - REL::Module::get().base());
            {
                struct RTTI_Code: Xbyak::CodeGenerator {
                    RTTI_Code() {
                        Xbyak::Label f_PrintRTTI;

                        push(rcx);
                        push(rdx);
                        push(rax);
                        mov(rcx, rdx);
                        sub(rsp, 0x8);// Ensure 16-byte alignment of stack pointer
                        sub(rsp, 0x20);
                        call(ptr[rip + f_PrintRTTI]);
                        add(rsp, 0x20);
                        add(rsp, 0x8);
                        pop(rax);
                        pop(rdx);
                        pop(rcx);

                        jmp(ptr[rip]);
                        dq(InventoryChanges_ExecuteVisitorOnWorn_Hook + 0x6);

                        L(f_PrintRTTI);
                        dq(uintptr_t(Print_RTTI));
                    }
                };
                RTTI_Code gen;
                void* code = g_localTrampoline->allocate(gen);

                LOG(info, "AVI: Patching RTTI print at addr = {:x}. base = {:x}",
                    InventoryChanges_ExecuteVisitorOnWorn_Hook, REL::Module::get().base());
                g_branchTrampoline->write_branch<6>(
                    InventoryChanges_ExecuteVisitorOnWorn_Hook, code);
            }
        }
    }// namespace RTTIPrinter

    void ApplyPlayerSkinningHooks() {
        DontVanillaSkinPlayer::Apply();
        ShimWornFlags::Apply();
        CustomSkinPlayer::Apply();
        FixEquipConflictCheck::Apply();
    }
}// namespace Hooking

#endif