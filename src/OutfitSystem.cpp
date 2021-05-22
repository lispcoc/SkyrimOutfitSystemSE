#include "OutfitSystem.h"

#pragma warning( push )
#pragma warning( disable : 4267 ) // SKSE has some integer conversion when returning arrays. Returned arrays should be limited to 32-bit size().
#pragma warning( disable : 5053 ) // SKSE uses explicit(<expr>) vendor extension.
#include "skse64/PapyrusNativeFunctions.h"
#include "RE/FormComponents/TESForm/TESObjectREFR/Actor/Character/PlayerCharacter.h"
#pragma warning( pop )

#include "skse64/PapyrusObjects.h"
#include "skse64/PapyrusVM.h"

#include "skse64/GameRTTI.h"
#include "skse64/GameFormComponents.h"
#include "skse64/GameObjects.h"
#include "skse64/GameReferences.h"

#pragma warning( push )
#pragma warning( disable : 5053 ) // CommonLibSSE uses explicit(<expr>) vendor extension.
#include "RE/FormComponents/TESForm/TESObject/TESBoundObject/TESObjectARMO.h"
#include "RE/FileIO/TESDataHandler.h"
#include "RE/FormComponents/TESForm/TESObjectREFR/Actor/Actor.h"
#include "RE/AI/AIProcess.h"
#include "RE/Inventory/InventoryChanges.h"
#include "RE/Inventory/InventoryEntryData.h"
#include "RE/FormComponents/TESForm/BGSLocation.h"
#include "RE/FormComponents/TESForm/TESWeather.h"
#include "RE/FormComponents/TESForm/BGSKeyword/BGSKeyword.h"
#include "RE/Misc/Misc.h"
#pragma warning( pop )

#include "ArmorAddonOverrideService.h"

#include "cobb/strings.h"
#include "cobb/utf8string.h"
#include "cobb/utf8naturalsort.h"

#include <algorithm>

#include "google/protobuf/util/json_util.h"

// Needed for save and load of config JSON
extern SKSESerializationInterface* g_Serialization;

    namespace OutfitSystem {
        SInt32 GetOutfitNameMaxLength(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*) {
            return ArmorAddonOverrideService::ce_outfitNameMaxLength;
        }
        VMResultArray<TESObjectARMO*> GetCarriedArmor(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, Actor* target_skse) {
            VMResultArray<RE::TESObjectARMO*> result;
            auto target = (RE::Actor*) (target_skse);
            if (target == nullptr) {
                registry->LogError("Cannot retrieve data for a None actor.", stackId);
                VMResultArray<TESObjectARMO*> empty;
                return empty;
            }
            //
            class _Visitor : public RE::InventoryChanges::IItemChangeVisitor {
                //
                // If the player has a shield equipped, and if we're not overriding that 
                // shield, then we need to grab the equipped shield's worn-flags.
                //
            public:
                virtual ReturnType Visit(RE::InventoryEntryData* data) override {
                    // Return true to continue, or else false to break.
                    const auto form = data->object;
                    if (form && form->formType == RE::FormType::Armor)
                        this->list.push_back(reinterpret_cast<RE::TESObjectARMO*>(form));
                    return ReturnType::kContinue;
                };

                VMResultArray<RE::TESObjectARMO*>& list;
                //
                _Visitor(VMResultArray<RE::TESObjectARMO*>& l) : list(l) {};
            };
            auto inventory = target->GetInventoryChanges();
            if (inventory) {
                _Visitor visitor(result);
                inventory->ExecuteVisitor(&visitor);
            }
            VMResultArray<TESObjectARMO*> converted_result;
            converted_result.reserve(result.size());
            for (const auto ptr : result) {
                converted_result.push_back((TESObjectARMO*)ptr);
            }
            return converted_result;
        }
        VMResultArray<TESObjectARMO*> GetWornItems(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, Actor* target_skse) {
            VMResultArray<RE::TESObjectARMO*> result;
            auto target = (RE::Actor*) (target_skse);
            if (target == nullptr) {
                registry->LogError("Cannot retrieve data for a None actor.", stackId);
                VMResultArray<TESObjectARMO*> empty;
                return empty;
            }
            //
            class _Visitor : public RE::InventoryChanges::IItemChangeVisitor {
                //
                // If the player has a shield equipped, and if we're not overriding that 
                // shield, then we need to grab the equipped shield's worn-flags.
                //
            public:
                virtual ReturnType Visit(RE::InventoryEntryData* data) override {
                    auto form = data->object;
                    if (form && form->formType == RE::FormType::Armor)
                        this->list.push_back(reinterpret_cast<RE::TESObjectARMO*>(form));
                    return ReturnType::kContinue;
                };

                VMResultArray<RE::TESObjectARMO*>& list;
                //
                _Visitor(VMResultArray<RE::TESObjectARMO*>& l) : list(l) {};
            };
            auto inventory = target->GetInventoryChanges();
            if (inventory) {
                _Visitor visitor(result);
                inventory->ExecuteVisitorOnWorn(&visitor);
            }
            VMResultArray<TESObjectARMO*> converted_result;
            converted_result.reserve(result.size());
            for (const auto ptr : result) {
                converted_result.push_back((TESObjectARMO*)ptr);
            }
            return converted_result;
        }
        void RefreshArmorFor(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, Actor* target_skse) {
            auto target = (RE::Actor*) (target_skse);
            ERROR_AND_RETURN_IF(target == nullptr, "Cannot refresh armor on a None actor.", registry, stackId);
            auto pm = target->currentProcess;
            if (pm) {
                //
                // "SetEquipFlag" tells the process manager that the actor's 
                // equipment has changed, and that their ArmorAddons should 
                // be updated. If you need to find it in Skyrim Special, you 
                // should see a call near the start of EquipManager's func-
                // tion to equip an item.
                //
                // NOTE: AIProcess is also called as ActorProcessManager
                pm->SetEquipFlag(RE::AIProcess::Flag::kUnk01);
                pm->UpdateEquipment(target);
            }
        }
        //
        namespace ArmorFormSearchUtils {
            static struct {
                std::vector<std::string>    names;
                std::vector<RE::TESObjectARMO*> armors;
                //
                void setup(std::string nameFilter, bool mustBePlayable) {
                    auto  data = RE::TESDataHandler::GetSingleton();
                    auto& list = data->GetFormArray(RE::FormType::Armor);
                    const auto  size = list.size();
                    this->names.reserve(size);
                    this->armors.reserve(size);
                    for (UInt32 i = 0; i < size; i++) {
                        const auto form = list[i];
                        if (form && form->formType == RE::FormType::Armor) {
                            auto armor = static_cast<RE::TESObjectARMO*>(form);
                            if (armor->templateArmor) // filter out predefined enchanted variants, to declutter the list
                                continue;
                            if (mustBePlayable && !!(armor->formFlags & RE::TESObjectARMO::RecordFlags::kNonPlayable))
                                continue;
                            std::string armorName;
                            {  // get name
                                // TESFullName* tfn = DYNAMIC_CAST(armor, TESObjectARMO, TESFullName);
                                TESFullName* tfn = (TESFullName*)Runtime_DynamicCast((void*)armor, RTTI_TESObjectARMO, RTTI_TESFullName);
                                if (tfn)
                                    armorName = tfn->name.data;
                            }
                            if (armorName.empty()) // skip nameless armor
                                continue;
                            if (!nameFilter.empty()) {
                                auto it = std::search(
                                    armorName.begin(), armorName.end(),
                                    nameFilter.begin(), nameFilter.end(),
                                    [](char a, char b) { return toupper(a) == toupper(b); }
                                );
                                if (it == armorName.end())
                                    continue;
                            }
                            this->armors.push_back(armor);
                            this->names.push_back(armorName.c_str());
                        }
                    }
                }
                void clear() {
                    this->names.clear();
                    this->armors.clear();
                }
            } data;
            //
            //
            void Prep(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString filter, bool mustBePlayable) {
                data.setup(filter.data, mustBePlayable);
            }
            VMResultArray<TESObjectARMO*> GetForms(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*) {
                VMResultArray<RE::TESObjectARMO*> result;
                auto& list = data.armors;
                for (auto it = list.begin(); it != list.end(); it++)
                    result.push_back(*it);
                VMResultArray<TESObjectARMO*> converted_result;
                converted_result.reserve(result.size());
                for (const auto ptr : result) {
                    converted_result.push_back((TESObjectARMO*)ptr);
                }
                return converted_result;
            }
            VMResultArray<BSFixedString> GetNames(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*) {
                VMResultArray<BSFixedString> result;
                auto& list = data.names;
                for (auto it = list.begin(); it != list.end(); it++)
                    result.push_back(it->c_str());
                return result;
            }
            void Clear(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*) {
                data.clear();
            }
        }
        namespace BodySlotListing {
            enum {
                kBodySlotMin = 30,
                kBodySlotMax = 61,
            };
            static struct {
                std::vector<SInt32>             bodySlots;
                std::vector<std::string>        armorNames;
                std::vector<RE::TESObjectARMO*> armors;
            } data;
            //
            void Clear(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*) {
                data.bodySlots.clear();
                data.armorNames.clear();
                data.armors.clear();
            }
            void Prep(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString name) {
                data.bodySlots.clear();
                data.armorNames.clear();
                data.armors.clear();
                //
                auto& service = ArmorAddonOverrideService::GetInstance();
                try {
                    auto& outfit = service.getOutfit(name.data);
                    auto& armors = outfit.armors;
                    for (UInt8 i = kBodySlotMin; i <= kBodySlotMax; i++) {
                        UInt32 mask = 1 << (i - kBodySlotMin);
                        for (auto it = armors.begin(); it != armors.end(); it++) {
                            RE::TESObjectARMO* armor = *it;
                            if (armor && (static_cast<UInt32>(armor->GetSlotMask()) & mask)) {
                                data.bodySlots.push_back(i);
                                data.armors.push_back(armor);
                                { // name
                                    // TESFullName* pFullName = DYNAMIC_CAST(armor, TESObjectARMO, TESFullName);
                                    TESFullName* pFullName = (TESFullName*) Runtime_DynamicCast((void*) armor, RTTI_TESObjectARMO, RTTI_TESFullName);
                                    if (pFullName)
                                        data.armorNames.push_back(pFullName->name.data);
                                    else
                                        data.armorNames.push_back("");
                                }
                            }
                        }
                    }
                }
                catch (std::out_of_range) {
                    registry->LogWarning("The specified outfit does not exist.", stackId);
                }
            }
            VMResultArray<TESObjectARMO*> GetArmorForms(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*) {
                VMResultArray<RE::TESObjectARMO*> result;
                auto& list = data.armors;
                for (auto it = list.begin(); it != list.end(); it++)
                    result.push_back(*it);
                VMResultArray<TESObjectARMO*> converted_result;
                converted_result.reserve(result.size());
                for (const auto ptr : result) {
                    converted_result.push_back((TESObjectARMO*)ptr);
                }
                return converted_result;
            }
            VMResultArray<BSFixedString> GetArmorNames(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*) {
                VMResultArray<BSFixedString> result;
                auto& list = data.armorNames;
                for (auto it = list.begin(); it != list.end(); it++)
                    result.push_back(it->c_str());
                return result;
            }
            VMResultArray<SInt32> GetSlotIndices(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*) {
                VMResultArray<SInt32> result;
                auto& list = data.bodySlots;
                for (auto it = list.begin(); it != list.end(); it++)
                    result.push_back(*it);
                return result;
            }
        }
        namespace StringSorts {
            VMResultArray<BSFixedString> NaturalSort_ASCII(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, VMArray<BSFixedString> arr, bool descending) {
                VMResultArray<BSFixedString> result;
                {  // Copy input array into output array
                    UInt32 size = arr.Length();
                    result.reserve(size);
                    for (UInt32 i = 0; i < size; i++) {
                        BSFixedString x;
                        arr.Get(&x, i);
                        result.push_back(x);
                    }
                }
                std::sort(
                    result.begin(),
                    result.end(),
                    [descending](const BSFixedString& x, const BSFixedString& y) {
                    std::string a(x.data);
                    std::string b(y.data);
                    if (descending)
                        std::swap(a, b);
                    return cobb::utf8::naturalcompare(a, b) > 0;
                }
                );
                return result;
            }
            template<typename T> VMResultArray<BSFixedString> NaturalSortPair_ASCII(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, VMArray<BSFixedString> arr, VMArray<T> second, bool descending) {
                UInt32 size = arr.Length();
                if (size != second.Length()) {
                    registry->LogError("The two arrays must be the same length.", stackId);
                    //
                    VMResultArray<BSFixedString> result;
                    result.reserve(size);
                    for (UInt32 i = 0; i < size; i++) {
                        BSFixedString x;
                        arr.Get(&x, i);
                        result.push_back(x);
                    }
                    return result;
                }
                //
                typedef std::pair<BSFixedString, T> _pair;
                std::vector<_pair> pairs;
                //
                VMResultArray<BSFixedString> result;
                {  // Copy input array into output array
                    result.reserve(size);
                    for (UInt32 i = 0; i < size; i++) {
                        BSFixedString x;
                        T y;
                        arr.Get(&x, i);
                        second.Get(&y, i);
                        pairs.emplace_back(x, y);
                    }
                }
                std::sort(
                    pairs.begin(),
                    pairs.end(),
                    [descending](const _pair& x, const _pair& y) {
                    auto result = cobb::utf8::naturalcompare(std::string(x.first.data), std::string(y.first.data));
                    if (descending)
                        result = -result;
                    return result > 0;
                }
                );
                for (UInt32 i = 0; i < size; i++) {
                    result.push_back(pairs[i].first);
                    second.Set(&pairs[i].second, i);
                }
                return result;
            }
        }
        namespace Utility {
            UInt32 HexToInt32(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString str) {
                const char* s = str.data;
                char* discard;
                return strtoul(s, &discard, 16);
            }
            BSFixedString ToHex(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, UInt32 value, SInt32 length) {
                if (length < 1) {
                    registry->LogWarning("Cannot format a hexadecimal valueinteger to a negative number of digits. Defaulting to eight.", stackId);
                    length = 8;
                }
                else if (length > 8) {
                    registry->LogWarning("Cannot format a hexadecimal integer longer than eight digits.", stackId);
                    length = 8;
                }
                char hex[9];
                memset(hex, '0', sizeof(hex));
                hex[length] = '\0';
                while (value > 0 && length--) {
                    UInt8 digit = value % 0x10;
                    value /= 0x10;
                    if (digit < 0xA) {
                        hex[length] = digit + '0';
                    }
                    else {
                        hex[length] = digit + 0x37;
                    }
                }
                return hex; // passes through BSFixedString constructor, which I believe caches the string, so returning local vars should be fine
            }
        }
        //
        void AddArmorToOutfit(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString name, TESObjectARMO* armor_skse) {
            auto armor = (RE::TESObjectARMO*) (armor_skse);
            ERROR_AND_RETURN_IF(armor == nullptr, "Cannot add a None armor to an outfit.", registry, stackId);
            auto& service = ArmorAddonOverrideService::GetInstance();
            try {
                auto& outfit = service.getOutfit(name.data);
                outfit.armors.insert(armor);
            }
            catch (std::out_of_range) {
                registry->LogWarning("The specified outfit does not exist.", stackId);
            }
        }
        bool ArmorConflictsWithOutfit(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, TESObjectARMO* armor_skse, BSFixedString name) {
            auto armor = (RE::TESObjectARMO*) (armor_skse);
            if (armor == nullptr) {
                registry->LogWarning("A None armor can't conflict with anything in an outfit.", stackId);
                return false;
            }
            auto& service = ArmorAddonOverrideService::GetInstance();
            try {
                auto& outfit = service.getOutfit(name.data);
                return outfit.conflictsWith(armor);
            }
            catch (std::out_of_range) {
                registry->LogWarning("The specified outfit does not exist.", stackId);
                return false;
            }
        }
        void CreateOutfit(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString name) {
            auto& service = ArmorAddonOverrideService::GetInstance();
            try {
                service.addOutfit(name.data);
            }
            catch (ArmorAddonOverrideService::bad_name) {
                registry->LogError("Invalid outfit name specified.", stackId);
                return;
            }
        }
        void DeleteOutfit(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString name) {
            auto& service = ArmorAddonOverrideService::GetInstance();
            service.deleteOutfit(name.data);
        }
        VMResultArray<TESObjectARMO*> GetOutfitContents(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString name) {
            VMResultArray<RE::TESObjectARMO*> result;
            auto& service = ArmorAddonOverrideService::GetInstance();
            try {
                auto& outfit = service.getOutfit(name.data);
                auto& armors = outfit.armors;
                for (auto it = armors.begin(); it != armors.end(); ++it)
                    result.push_back(*it);
            }
            catch (std::out_of_range) {
                registry->LogWarning("The specified outfit does not exist.", stackId);
            }
            VMResultArray<TESObjectARMO*> converted_result;
            converted_result.reserve(result.size());
            for (const auto ptr : result) {
                converted_result.push_back((TESObjectARMO*)ptr);
            }
            return converted_result;
        }
        bool GetOutfitFavoriteStatus(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString name) {
            auto& service = ArmorAddonOverrideService::GetInstance();
            bool result = false;
            try {
                auto& outfit = service.getOutfit(name.data);
                result = outfit.isFavorite;
            }
            catch (std::out_of_range) {
                registry->LogWarning("The specified outfit does not exist.", stackId);
            }
            return result;
        }
        bool GetOutfitPassthroughStatus(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString name) {
            auto& service = ArmorAddonOverrideService::GetInstance();
            bool result = false;
            try {
                auto& outfit = service.getOutfit(name.data);
                result = outfit.allowsPassthrough;
            }
            catch (std::out_of_range) {
                registry->LogWarning("The specified outfit does not exist.", stackId);
            }
            return result;
        }
        bool GetOutfitEquipRequiredStatus(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString name) {
            auto& service = ArmorAddonOverrideService::GetInstance();
            bool result = false;
            try {
                auto& outfit = service.getOutfit(name.data);
                result = outfit.requiresEquipped;
            }
            catch (std::out_of_range) {
                registry->LogWarning("The specified outfit does not exist.", stackId);
            }
            return result;
        }
    BSFixedString GetSelectedOutfit(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*) {
            auto& service = ArmorAddonOverrideService::GetInstance();
            return service.currentOutfit(RE::PlayerCharacter::GetSingleton()).name.c_str();
        }
        bool IsEnabled(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*) {
            auto& service = ArmorAddonOverrideService::GetInstance();
            return service.enabled;
        }
        VMResultArray<BSFixedString> ListOutfits(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, bool favoritesOnly) {
            auto& service = ArmorAddonOverrideService::GetInstance();
            VMResultArray<BSFixedString> result;
            std::vector<std::string> intermediate;
            service.getOutfitNames(intermediate, favoritesOnly);
            result.reserve(intermediate.size());
            for (auto it = intermediate.begin(); it != intermediate.end(); ++it)
                result.push_back(it->c_str());
            return result;
        }
        void RemoveArmorFromOutfit(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString name, TESObjectARMO* armor_skse) {
            auto armor = (RE::TESObjectARMO*) (armor_skse);
            ERROR_AND_RETURN_IF(armor == nullptr, "Cannot remove a None armor from an outfit.", registry, stackId);
            auto& service = ArmorAddonOverrideService::GetInstance();
            try {
                auto& outfit = service.getOutfit(name.data);
                outfit.armors.erase(armor);
            }
            catch (std::out_of_range) {
                registry->LogWarning("The specified outfit does not exist.", stackId);
            }
        }
        void RemoveConflictingArmorsFrom(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, TESObjectARMO* armor_skse, BSFixedString name) {
            auto armor = (RE::TESObjectARMO*) (armor_skse);
            ERROR_AND_RETURN_IF(armor == nullptr, "A None armor can't conflict with anything in an outfit.", registry, stackId);
            auto& service = ArmorAddonOverrideService::GetInstance();
            try {
                auto& outfit = service.getOutfit(name.data);
                auto& armors = outfit.armors;
                std::vector<RE::TESObjectARMO*> conflicts;
                const auto candidateMask = armor->GetSlotMask();
                for (auto it = armors.begin(); it != armors.end(); ++it) {
                    RE::TESObjectARMO* existing = *it;
                    if (existing) {
                        const auto mask = existing->GetSlotMask();
                        if ((static_cast<uint32_t>(mask) & static_cast<uint32_t>(candidateMask)) != static_cast<uint32_t>(RE::BGSBipedObjectForm::FirstPersonFlag::kNone))
                            conflicts.push_back(existing);
                    }
                }
                for (auto it = conflicts.begin(); it != conflicts.end(); ++it)
                    armors.erase(*it);
            }
            catch (std::out_of_range) {
                registry->LogError("The specified outfit does not exist.", stackId);
                return;
            }
        }
        bool RenameOutfit(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString name, BSFixedString changeTo) {
            auto& service = ArmorAddonOverrideService::GetInstance();
            try {
                service.renameOutfit(name.data, changeTo.data);
            }
            catch (ArmorAddonOverrideService::bad_name) {
                registry->LogError("The desired name is invalid.", stackId);
                return false;
            }
            catch (ArmorAddonOverrideService::name_conflict) {
                registry->LogError("The desired name is taken.", stackId);
                return false;
            }
            catch (std::out_of_range) {
                registry->LogError("The specified outfit does not exist.", stackId);
                return false;
            }
            return true;
        }
        void SetOutfitFavoriteStatus(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString name, bool favorite) {
            auto& service = ArmorAddonOverrideService::GetInstance();
            service.setFavorite(name.data, favorite);
        }
        void SetOutfitPassthroughStatus(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString name, bool allowsPassthrough) {
            auto& service = ArmorAddonOverrideService::GetInstance();
            service.setOutfitPassthrough(name.data, allowsPassthrough);
        }
        void SetOutfitEquipRequiredStatus(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString name, bool equipRequired) {
            auto& service = ArmorAddonOverrideService::GetInstance();
            service.setOutfitEquipRequired(name.data, equipRequired);
        }
    bool OutfitExists(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString name) {
            auto& service = ArmorAddonOverrideService::GetInstance();
            return service.hasOutfit(name.data);
        }
        void OverwriteOutfit(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString name, VMArray<TESObjectARMO*> armors_skse) {
            auto& service = ArmorAddonOverrideService::GetInstance();
            // Convert input array.
            VMResultArray<RE::TESObjectARMO*> armors;
            armors.reserve(armors_skse.Length());
            for (std::size_t i = 0; i < armors_skse.Length(); i++) {
                TESObjectARMO* ptr;
                armors_skse.Get(&ptr, static_cast<UInt32>(i));
                armors.push_back((RE::TESObjectARMO*)ptr);
            }
            // End convert
            try {
                auto& outfit = service.getOrCreateOutfit(name.data);
                outfit.armors.clear();
                auto count = armors.size();
                for (UInt32 i = 0; i < count; i++) {
                    RE::TESObjectARMO* ptr = nullptr;
                    ptr = armors.at(i);
                    if (ptr)
                        outfit.armors.insert(ptr);
                }
            }
            catch (ArmorAddonOverrideService::bad_name) {
                registry->LogError("Invalid outfit name specified.", stackId);
                return;
            }
        }
        void SetEnabled(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, bool state) {
            auto& service = ArmorAddonOverrideService::GetInstance();
            service.setEnabled(state);
        }
        void SetSelectedOutfit(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BSFixedString name) {
            auto& service = ArmorAddonOverrideService::GetInstance();
            service.setOutfit(name.data, RE::PlayerCharacter::GetSingleton());
        }
        void AddActor(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, Actor* target) {
            auto& service = ArmorAddonOverrideService::GetInstance();
            service.addActor((RE::Actor*) target);
        }
        void RemoveActor(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, Actor* target) {
            auto& service = ArmorAddonOverrideService::GetInstance();
            service.removeActor((RE::Actor*) target);
        }
        void SetLocationBasedAutoSwitchEnabled(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, bool value) {
            ArmorAddonOverrideService::GetInstance().setLocationBasedAutoSwitchEnabled(value);
        }
        bool GetLocationBasedAutoSwitchEnabled(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*) {
            return ArmorAddonOverrideService::GetInstance().locationBasedAutoSwitchEnabled;
        }
        VMResultArray<UInt32> GetAutoSwitchLocationArray(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*) {
            VMResultArray<UInt32> result;
            for (LocationType i : {
                    LocationType::World,
                    LocationType::WorldSnowy,
                    LocationType::WorldRainy,
                    LocationType::City,
                    LocationType::CitySnowy,
                    LocationType::CityRainy,
                    LocationType::Town,
                    LocationType::TownSnowy,
                    LocationType::TownRainy,
                    LocationType::Dungeon,
                    LocationType::DungeonSnowy,
                    LocationType::DungeonRainy
            }) {
                result.push_back(UInt32(i));
            }
            return result;
        }
        std::optional<LocationType> identifyLocation(RE::BGSLocation* location, RE::TESWeather* weather) {
            // Just a helper function to classify a location.
            // TODO: Think of a better place than this since we're not exposing it to Papyrus.
            auto& service = ArmorAddonOverrideService::GetInstance();

            // Collect weather information.
            WeatherFlags weather_flags;
            if (weather) {
                weather_flags.snowy = weather->data.flags.any(RE::TESWeather::WeatherDataFlag::kSnow);
                weather_flags.rainy = weather->data.flags.any(RE::TESWeather::WeatherDataFlag::kRainy);
            }

            // Collect location keywords
            std::unordered_set<std::string> keywords;
            keywords.reserve(20);
            while (location) {
                std::uint32_t max = location->GetNumKeywords();
                for (std::uint32_t i = 0; i < max; i++) {
                    RE::BGSKeyword* keyword = location->GetKeywordAt(i).value();
                    /*
                    char message[100];
                    _MESSAGE("SOS: Location has Keyword %s", keyword->GetFormEditorID());
                    sprintf(message, "SOS: Location has keyword %s", keyword->GetFormEditorID());
                    RE::DebugNotification(message, nullptr, false);
                    */
                    keywords.emplace(keyword->GetFormEditorID());
                }
                location = location->parentLoc;
            }
            return service.checkLocationType(keywords, weather_flags, RE::PlayerCharacter::GetSingleton());
        }
        UInt32 IdentifyLocationType(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BGSLocation* location_skse, TESWeather* weather_skse) {
            // NOTE: Identify the location for Papyrus. In the event no location is identified, we lie to Papyrus and say "World".
            //       Therefore, Papyrus cannot assume that locations returned have an outfit assigned, at least not for "World".
            return static_cast<UInt32>(identifyLocation((RE::BGSLocation*) location_skse, (RE::TESWeather*) weather_skse).value_or(LocationType::World));
        }
        void SetOutfitUsingLocation(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, BGSLocation* location_skse, TESWeather* weather_skse) {
            // NOTE: Location can be NULL.
            auto& service = ArmorAddonOverrideService::GetInstance();

            if (service.locationBasedAutoSwitchEnabled) {
                auto location = identifyLocation((RE::BGSLocation*) location_skse, (RE::TESWeather*) weather_skse);
                // Debug notifications for location classification.
                /*
                const char* locationName = locationTypeStrings[static_cast<std::uint32_t>(location)];
                char message[100];
                sprintf_s(message, "SOS: This location is a %s.", locationName);
                RE::DebugNotification(message, nullptr, false);
                */
                if (location.has_value()) {
                    service.setOutfitUsingLocation(location.value(), RE::PlayerCharacter::GetSingleton());
                }
            }

        }
        void SetLocationOutfit(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, UInt32 location, BSFixedString name) {
            if (strcmp(name.data, "") == 0) {
                // Location outfit assignment is never allowed to be empty string. Use unset instead.
                return;
            }
            return ArmorAddonOverrideService::GetInstance().setLocationOutfit(LocationType(location), name.data, RE::PlayerCharacter::GetSingleton());
        }
        void UnsetLocationOutfit(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, UInt32 location) {
            return ArmorAddonOverrideService::GetInstance().unsetLocationOutfit(LocationType(location), RE::PlayerCharacter::GetSingleton());
        }
        BSFixedString GetLocationOutfit(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*, UInt32 location) {
            auto outfit = ArmorAddonOverrideService::GetInstance().getLocationOutfit(LocationType(location), RE::PlayerCharacter::GetSingleton());
            if (outfit.has_value()) {
                return BSFixedString(outfit.value().c_str());
            } else {
                // Empty string means "no outfit assigned" for this location type.
                return BSFixedString("");
            }
        }
        bool ExportSettings(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*) {
            std::string	outputFile = GetRuntimeDirectory() + "Data\\SKSE\\Plugins\\OutfitSystemData.json";
            auto& service = ArmorAddonOverrideService::GetInstance();
            proto::OutfitSystem data = service.save(g_Serialization);
            std::string output;
            google::protobuf::util::JsonPrintOptions options;
            options.add_whitespace = true;
            google::protobuf::util::MessageToJsonString(data, &output, options);
            std::ofstream file(outputFile);
            if (file) {
                file << output;
            } else {
                RE::DebugNotification("Failed to open config for writing", nullptr, false);
                return false;
            }
            if (file.good()) {
                std::string message = "Wrote JSON config to " + outputFile;
                RE::DebugNotification(message.c_str(), nullptr, false);
                return true;
            } else {
                RE::DebugNotification("Failed to write config", nullptr, false);
                return false;
            }
        }
        bool ImportSettings(VMClassRegistry* registry, UInt32 stackId, StaticFunctionTag*) {
            std::string	inputFile = GetRuntimeDirectory() + "Data\\SKSE\\Plugins\\OutfitSystemData.json";
            std::ifstream file(inputFile);
            if (!file) {
                RE::DebugNotification("Failed to open config for reading", nullptr, false);
                return false;
            }
            std::stringstream input;
            input << file.rdbuf();
            if (!file.good()) {
                RE::DebugNotification("Failed to read config data", nullptr, false);
                return false;
            }
            proto::OutfitSystem data;
            auto status = google::protobuf::util::JsonStringToMessage(input.str(), &data);
            if (!status.ok()) {
                RE::DebugNotification("Failed to parse config data. Invalid syntax.", nullptr, false);
                return false;
            }
            auto& service = ArmorAddonOverrideService::GetInstance();
            service.load(g_Serialization, data);
            std::string message = "Read JSON config from " + inputFile;
            RE::DebugNotification(message.c_str(), nullptr, false);
            return true;
        }
    }


bool OutfitSystem::RegisterPapyrus(VMClassRegistry* registry) {
    registry->RegisterFunction(new NativeFunction0<StaticFunctionTag, SInt32>(
        "GetOutfitNameMaxLength",
        "SkyrimOutfitSystemNativeFuncs",
        GetOutfitNameMaxLength,
        registry
        ));
    registry->SetFunctionFlags("SkyrimOutfitSystemNativeFuncs", "GetOutfitNameMaxLength", VMClassRegistry::kFunctionFlag_NoWait);
    registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, VMResultArray<TESObjectARMO*>, Actor*>(
        "GetCarriedArmor",
        "SkyrimOutfitSystemNativeFuncs",
        GetCarriedArmor,
        registry
        ));
    registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, VMResultArray<TESObjectARMO*>, Actor*>(
        "GetWornItems",
        "SkyrimOutfitSystemNativeFuncs",
        GetWornItems,
        registry
        ));
    registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, Actor*>(
        "RefreshArmorFor",
        "SkyrimOutfitSystemNativeFuncs",
        RefreshArmorFor,
        registry
        ));
    //
    {  // armor form search utils
        registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, BSFixedString, bool>(
            "PrepArmorSearch",
            "SkyrimOutfitSystemNativeFuncs",
            ArmorFormSearchUtils::Prep,
            registry
            ));
        registry->RegisterFunction(new NativeFunction0<StaticFunctionTag, VMResultArray<TESObjectARMO*>>(
            "GetArmorSearchResultForms",
            "SkyrimOutfitSystemNativeFuncs",
            ArmorFormSearchUtils::GetForms,
            registry
            ));
        registry->RegisterFunction(new NativeFunction0<StaticFunctionTag, VMResultArray<BSFixedString>>(
            "GetArmorSearchResultNames",
            "SkyrimOutfitSystemNativeFuncs",
            ArmorFormSearchUtils::GetNames,
            registry
            ));
        registry->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>(
            "ClearArmorSearch",
            "SkyrimOutfitSystemNativeFuncs",
            ArmorFormSearchUtils::Clear,
            registry
            ));
    }
    {  // body slot data
        registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, BSFixedString>(
            "PrepOutfitBodySlotListing",
            "SkyrimOutfitSystemNativeFuncs",
            BodySlotListing::Prep,
            registry
            ));
        registry->RegisterFunction(new NativeFunction0<StaticFunctionTag, VMResultArray<TESObjectARMO*>>(
            "GetOutfitBodySlotListingArmorForms",
            "SkyrimOutfitSystemNativeFuncs",
            BodySlotListing::GetArmorForms,
            registry
            ));
        registry->RegisterFunction(new NativeFunction0<StaticFunctionTag, VMResultArray<BSFixedString>>(
            "GetOutfitBodySlotListingArmorNames",
            "SkyrimOutfitSystemNativeFuncs",
            BodySlotListing::GetArmorNames,
            registry
            ));
        registry->RegisterFunction(new NativeFunction0<StaticFunctionTag, VMResultArray<SInt32>>(
            "GetOutfitBodySlotListingSlotIndices",
            "SkyrimOutfitSystemNativeFuncs",
            BodySlotListing::GetSlotIndices,
            registry
            ));
        registry->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>(
            "ClearOutfitBodySlotListing",
            "SkyrimOutfitSystemNativeFuncs",
            BodySlotListing::Clear,
            registry
            ));
    }
    {  // string sorts
        registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, VMResultArray<BSFixedString>, VMArray<BSFixedString>, bool>(
            "NaturalSort_ASCII",
            "SkyrimOutfitSystemNativeFuncs",
            StringSorts::NaturalSort_ASCII,
            registry
            ));
        registry->SetFunctionFlags("SkyrimOutfitSystemNativeFuncs", "NaturalSort_ASCII", VMClassRegistry::kFunctionFlag_NoWait);
        registry->RegisterFunction(new NativeFunction3<StaticFunctionTag, VMResultArray<BSFixedString>, VMArray<BSFixedString>, VMArray<TESObjectARMO*>, bool>(
            "NaturalSortPairArmor_ASCII",
            "SkyrimOutfitSystemNativeFuncs",
            StringSorts::NaturalSortPair_ASCII<TESObjectARMO*>,
            registry
            ));
        registry->SetFunctionFlags("SkyrimOutfitSystemNativeFuncs", "NaturalSortPairArmor_ASCII", VMClassRegistry::kFunctionFlag_NoWait);
    }
    {  // Utility
        registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, UInt32, BSFixedString>(
            "HexToInt32",
            "SkyrimOutfitSystemNativeFuncs",
            Utility::HexToInt32,
            registry
            ));
        registry->SetFunctionFlags("SkyrimOutfitSystemNativeFuncs", "HexToInt32", VMClassRegistry::kFunctionFlag_NoWait);
        registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, BSFixedString, UInt32, SInt32>(
            "ToHex",
            "SkyrimOutfitSystemNativeFuncs",
            Utility::ToHex,
            registry
            ));
        registry->SetFunctionFlags("SkyrimOutfitSystemNativeFuncs", "ToHex", VMClassRegistry::kFunctionFlag_NoWait);
    }
    //
    registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, BSFixedString, TESObjectARMO*>(
        "AddArmorToOutfit",
        "SkyrimOutfitSystemNativeFuncs",
        AddArmorToOutfit,
        registry
        ));
    registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, bool, TESObjectARMO*, BSFixedString>(
        "ArmorConflictsWithOutfit",
        "SkyrimOutfitSystemNativeFuncs",
        ArmorConflictsWithOutfit,
        registry
        ));
    registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, BSFixedString>(
        "CreateOutfit",
        "SkyrimOutfitSystemNativeFuncs",
        CreateOutfit,
        registry
        ));
    registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, BSFixedString>(
        "DeleteOutfit",
        "SkyrimOutfitSystemNativeFuncs",
        DeleteOutfit,
        registry
        ));
    registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, VMResultArray<TESObjectARMO*>, BSFixedString>(
        "GetOutfitContents",
        "SkyrimOutfitSystemNativeFuncs",
        GetOutfitContents,
        registry
        ));
    registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, bool, BSFixedString>(
        "GetOutfitFavoriteStatus",
        "SkyrimOutfitSystemNativeFuncs",
        GetOutfitFavoriteStatus,
        registry
    ));
    registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, bool, BSFixedString>(
            "GetOutfitPassthroughStatus",
            "SkyrimOutfitSystemNativeFuncs",
            GetOutfitPassthroughStatus,
            registry
    ));
    registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, bool, BSFixedString>(
            "GetOutfitEquipRequiredStatus",
            "SkyrimOutfitSystemNativeFuncs",
            GetOutfitEquipRequiredStatus,
            registry
    ));
    registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, BSFixedString, bool>(
        "SetOutfitFavoriteStatus",
        "SkyrimOutfitSystemNativeFuncs",
        SetOutfitFavoriteStatus,
        registry
    ));
    registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, BSFixedString, bool>(
            "SetOutfitPassthroughStatus",
            "SkyrimOutfitSystemNativeFuncs",
            SetOutfitPassthroughStatus,
            registry
    ));
    registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, BSFixedString, bool>(
            "SetOutfitEquipRequiredStatus",
            "SkyrimOutfitSystemNativeFuncs",
            SetOutfitEquipRequiredStatus,
            registry
    ));
    registry->RegisterFunction(new NativeFunction0<StaticFunctionTag, bool>(
        "IsEnabled",
        "SkyrimOutfitSystemNativeFuncs",
        IsEnabled,
        registry
        ));
    registry->RegisterFunction(new NativeFunction0<StaticFunctionTag, BSFixedString>(
        "GetSelectedOutfit",
        "SkyrimOutfitSystemNativeFuncs",
        GetSelectedOutfit,
        registry
        ));
    registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, VMResultArray<BSFixedString>, bool>(
        "ListOutfits",
        "SkyrimOutfitSystemNativeFuncs",
        ListOutfits,
        registry
        ));
    registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, BSFixedString, TESObjectARMO*>(
        "RemoveArmorFromOutfit",
        "SkyrimOutfitSystemNativeFuncs",
        RemoveArmorFromOutfit,
        registry
        ));
    registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, TESObjectARMO*, BSFixedString>(
        "RemoveConflictingArmorsFrom",
        "SkyrimOutfitSystemNativeFuncs",
        RemoveConflictingArmorsFrom,
        registry
        ));
    registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, bool, BSFixedString, BSFixedString>(
        "RenameOutfit",
        "SkyrimOutfitSystemNativeFuncs",
        RenameOutfit,
        registry
        ));
    registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, bool, BSFixedString>(
        "OutfitExists",
        "SkyrimOutfitSystemNativeFuncs",
        OutfitExists,
        registry
        ));
    registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, BSFixedString, VMArray<TESObjectARMO*>>(
        "OverwriteOutfit",
        "SkyrimOutfitSystemNativeFuncs",
        OverwriteOutfit,
        registry
        ));
    registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, bool>(
        "SetEnabled",
        "SkyrimOutfitSystemNativeFuncs",
        SetEnabled,
        registry
        ));
    registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, BSFixedString>(
        "SetSelectedOutfit",
        "SkyrimOutfitSystemNativeFuncs",
        SetSelectedOutfit,
        registry
        ));
    registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, Actor*>(
        "AddActor",
        "SkyrimOutfitSystemNativeFuncs",
        AddActor,
        registry
    ));
    registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, Actor*>(
        "RemoveActor",
        "SkyrimOutfitSystemNativeFuncs",
        RemoveActor,
        registry
    ));
    registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, bool>(
        "SetLocationBasedAutoSwitchEnabled",
        "SkyrimOutfitSystemNativeFuncs",
        SetLocationBasedAutoSwitchEnabled,
        registry
        ));
    registry->RegisterFunction(new NativeFunction0<StaticFunctionTag, bool>(
        "GetLocationBasedAutoSwitchEnabled",
        "SkyrimOutfitSystemNativeFuncs",
        GetLocationBasedAutoSwitchEnabled,
        registry
    ));
    registry->RegisterFunction(new NativeFunction0<StaticFunctionTag, VMResultArray<UInt32>>(
        "GetAutoSwitchLocationArray",
        "SkyrimOutfitSystemNativeFuncs",
        GetAutoSwitchLocationArray,
        registry
    ));
    registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, UInt32, BGSLocation*, TESWeather*>(
        "IdentifyLocationType",
        "SkyrimOutfitSystemNativeFuncs",
        IdentifyLocationType,
        registry
    ));
    registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, BGSLocation*, TESWeather*>(
        "SetOutfitUsingLocation",
        "SkyrimOutfitSystemNativeFuncs",
        SetOutfitUsingLocation,
        registry
        ));
    registry->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, UInt32, BSFixedString>(
        "SetLocationOutfit",
        "SkyrimOutfitSystemNativeFuncs",
        SetLocationOutfit,
        registry
        ));
    registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, UInt32>(
        "UnsetLocationOutfit",
        "SkyrimOutfitSystemNativeFuncs",
        UnsetLocationOutfit,
        registry
        ));
    registry->RegisterFunction(new NativeFunction1<StaticFunctionTag, BSFixedString, UInt32>(
        "GetLocationOutfit",
        "SkyrimOutfitSystemNativeFuncs",
        GetLocationOutfit,
        registry
        ));
    registry->RegisterFunction(new NativeFunction0<StaticFunctionTag, bool>(
        "ExportSettings",
        "SkyrimOutfitSystemNativeFuncs",
        ExportSettings,
        registry
    ));
    registry->RegisterFunction(new NativeFunction0<StaticFunctionTag, bool>(
        "ImportSettings",
        "SkyrimOutfitSystemNativeFuncs",
        ImportSettings,
        registry
    ));

    return true;
}