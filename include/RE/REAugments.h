//
// Created by m on 10/8/2022.
//

#ifndef SKYRIMOUTFITSYSTEMSE_INCLUDE_RE_REAUGMENTS_H
#define SKYRIMOUTFITSYSTEMSE_INCLUDE_RE_REAUGMENTS_H

namespace RE {
    class ActorWeightModel;

    class IItemChangeVisitorAugment {
    public:
        inline static constexpr auto RTTI = RTTI_InventoryChanges__IItemChangeVisitor;

        virtual ~IItemChangeVisitorAugment() {};  // 00

        enum VisitorReturn: std::uint32_t {
            kStop,
            kContinue
        };

        // add
        virtual VisitorReturn Visit(InventoryEntryData* a_entryData) = 0;  // 01
        virtual void Unk_02(void) {};                                      // 02 - { return 1; }
        virtual void Unk_03(void) {};                                      // 03
    };
    static_assert(sizeof(IItemChangeVisitorAugment) == 0x8);


    namespace InventoryChangesAugments {
        void ExecuteVisitor(RE::InventoryChanges* thisPtr, RE::InventoryChanges::IItemChangeVisitor * a_visitor);
        void ExecuteAugmentVisitor(RE::InventoryChanges* thisPtr, RE::IItemChangeVisitorAugment * a_visitor);
        void ExecuteVisitorOnWorn(RE::InventoryChanges* thisPtr, RE::InventoryChanges::IItemChangeVisitor * a_visitor);
        void ExecuteAugmentVisitorOnWorn(RE::InventoryChanges* thisPtr, RE::IItemChangeVisitorAugment * a_visitor);
    }

    namespace AIProcessAugments {
        enum class Flag : std::uint8_t
        {
            kNone = 0,
            kUnk01 = 1 << 0,
            kUnk02 = 1 << 1,
            kUnk03 = 1 << 2,
            kDrawHead = 1 << 3,
            kMobile = 1 << 4,
            kReset = 1 << 5
        };
        void SetEquipFlag(RE::AIProcess* thisPtr, Flag a_flag);
        void UpdateEquipment(RE::AIProcess* thisPtr, Actor* a_actor);
    }

    namespace TESObjectARMOAugments {
        bool ApplyArmorAddon(RE::TESObjectARMO* thisPtr, TESRace* a_race, ActorWeightModel* a_model, bool a_isFemale);
        bool TestBodyPartByIndex(RE::TESObjectARMO* thisPtr, std::uint32_t a_index);
    }
}

#endif //SKYRIMOUTFITSYSTEMSE_INCLUDE_RE_REAUGMENTS_H
