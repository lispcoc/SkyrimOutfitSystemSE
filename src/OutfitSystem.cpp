#include "OutfitSystem.h"

#include "Utility.h"

#include "ArmorAddonOverrideService.h"

#include "RE/REAugments.h"
#include "cobb/strings.h"
#include "cobb/utf8naturalsort.h"
#include "cobb/utf8string.h"

#include <algorithm>

#include "google/protobuf/util/json_util.h"

#define ERROR_AND_RETURN_EXPR_IF(condition, message, valueExpr, registry, stackId)               \
    if (condition) {                                                                             \
        registry->TraceStack(message, stackId, RE::BSScript::IVirtualMachine::Severity::kError); \
        return (valueExpr);                                                                      \
    }

#define ERROR_AND_RETURN_IF(condition, message, registry, stackId)                               \
    if (condition) {                                                                             \
        registry->TraceStack(message, stackId, RE::BSScript::IVirtualMachine::Severity::kError); \
        return;                                                                                  \
    }

namespace OutfitSystem {
    std::int32_t GetOutfitNameMaxLength(RE::BSScript::IVirtualMachine* registry,
                                        std::uint32_t stackId,
                                        RE::StaticFunctionTag*) {
        LogExit exitPrint("GetOutfitNameMaxLength"sv);
        return ArmorAddonOverrideService::ce_outfitNameMaxLength;
    }
    std::vector<RE::TESObjectARMO*> GetCarriedArmor(RE::BSScript::IVirtualMachine* registry,
                                                    std::uint32_t stackId,
                                                    RE::StaticFunctionTag*,
                                                    RE::Actor* target) {
        LogExit exitPrint("GetCarriedArmor"sv);
        std::vector<RE::TESObjectARMO*> result;
        if (target == nullptr) {
            registry->TraceStack("Cannot retrieve data for a None RE::Actor.",
                                 stackId,
                                 RE::BSScript::IVirtualMachine::Severity::kError);
            std::vector<RE::TESObjectARMO*> empty;
            return empty;
        }
        //
        class _Visitor: public RE::IItemChangeVisitorAugment {
            //
            // If the player has a shield equipped, and if we're not overriding that
            // shield, then we need to grab the equipped shield's worn-flags.
            //
        public:
            virtual VisitorReturn Visit(RE::InventoryEntryData* data) override {
                // Return true to continue, or else false to break.
                const auto form = data->object;
                if (form && form->formType == RE::FormType::Armor) {
                    auto armor = skyrim_cast<RE::TESObjectARMO*>(form);
                    if (armor) this->list.push_back(armor);
                }
                return VisitorReturn::kContinue;
            };

            std::vector<RE::TESObjectARMO*>& list;
            //
            _Visitor(std::vector<RE::TESObjectARMO*>& l) : list(l){};
        };
        auto inventory = target->GetInventoryChanges();
        if (inventory) {
            _Visitor visitor(result);
            RE::InventoryChangesAugments::ExecuteAugmentVisitorOnWorn(inventory, &visitor);
        }
        std::vector<RE::TESObjectARMO*> converted_result;
        converted_result.reserve(result.size());
        for (const auto ptr : result) {
            converted_result.push_back((RE::TESObjectARMO*) ptr);
        }
        return converted_result;
    }
    std::vector<RE::TESObjectARMO*> GetWornItems(
        RE::BSScript::IVirtualMachine* registry,
        std::uint32_t stackId,
        RE::StaticFunctionTag*,
        RE::Actor* target) {
        LogExit exitPrint("GetWornItems"sv);
        std::vector<RE::TESObjectARMO*> result;
        if (target == nullptr) {
            registry->TraceStack("Cannot retrieve data for a None RE::Actor.", stackId, RE::BSScript::IVirtualMachine::Severity::kError);
            std::vector<RE::TESObjectARMO*> empty;
            return empty;
        }
        //
        class _Visitor: public RE::IItemChangeVisitorAugment {
            //
            // If the player has a shield equipped, and if we're not overriding that
            // shield, then we need to grab the equipped shield's worn-flags.
            //
        public:
            virtual VisitorReturn Visit(RE::InventoryEntryData* data) override {
                auto form = data->object;
                if (form && form->formType == RE::FormType::Armor) {
                    auto armor = skyrim_cast<RE::TESObjectARMO*>(form);
                    if (armor) this->list.push_back(armor);
                }
                return VisitorReturn::kContinue;
            };

            std::vector<RE::TESObjectARMO*>& list;
            //
            _Visitor(std::vector<RE::TESObjectARMO*>& l) : list(l){};
        };
        auto inventory = target->GetInventoryChanges();
        if (inventory) {
            _Visitor visitor(result);
            RE::InventoryChangesAugments::ExecuteAugmentVisitorOnWorn(inventory, &visitor);
        }
        std::vector<RE::TESObjectARMO*> converted_result;
        converted_result.reserve(result.size());
        for (const auto ptr : result) {
            converted_result.push_back((RE::TESObjectARMO*) ptr);
        }
        return converted_result;
    }
    void RefreshArmorFor(RE::BSScript::IVirtualMachine* registry,
                         std::uint32_t stackId,
                         RE::StaticFunctionTag*,
                         RE::Actor* target) {
        LogExit exitPrint("RefreshArmorFor"sv);
        ERROR_AND_RETURN_IF(target == nullptr, "Cannot refresh armor on a None RE::Actor.", registry, stackId);
        auto pm = target->GetActorRuntimeData().currentProcess;
        if (pm) {
            //
            // "SetEquipFlag" tells the process manager that the RE::Actor's
            // equipment has changed, and that their ArmorAddons should
            // be updated. If you need to find it in Skyrim Special, you
            // should see a call near the start of EquipManager's func-
            // tion to equip an item.
            //
            // NOTE: AIProcess is also called as RE::ActorProcessManager
            RE::AIProcessAugments::SetEquipFlag(pm, RE::AIProcessAugments::Flag::kUnk01);
            RE::AIProcessAugments::UpdateEquipment(pm, target);
        }
    }
    void RefreshArmorForAllConfiguredActors(RE::BSScript::IVirtualMachine* registry,
                                            std::uint32_t stackId,
                                            RE::StaticFunctionTag*) {
        LogExit exitPrint("RefreshArmorForAllConfiguredActors"sv);
        auto& service = ArmorAddonOverrideService::GetInstance();
        auto actors = service.listActors();
        for (auto& actorRef : actors) {
            auto actor = RE::Actor::LookupByHandle(actorRef);
            if (!actor)
                continue;
            auto pm = actor->GetActorRuntimeData().currentProcess;
            if (pm) {
                //
                // "SetEquipFlag" tells the process manager that the RE::Actor's
                // equipment has changed, and that their ArmorAddons should
                // be updated. If you need to find it in Skyrim Special, you
                // should see a call near the start of EquipManager's func-
                // tion to equip an item.
                //
                // NOTE: AIProcess is also called as RE::ActorProcessManager
                RE::AIProcessAugments::SetEquipFlag(pm, RE::AIProcessAugments::Flag::kUnk01);
                RE::AIProcessAugments::UpdateEquipment(pm, actor.get());
            }
        }
    }

    std::vector<RE::Actor*> ActorsNearPC(RE::BSScript::IVirtualMachine* registry,
                                         std::uint32_t stackId,
                                         RE::StaticFunctionTag*) {
        LogExit exitPrint("RefreshArmorForAllConfiguredActors"sv);
        std::vector<RE::Actor*> result;
        auto pc = RE::PlayerCharacter::GetSingleton();
        ERROR_AND_RETURN_EXPR_IF(pc == nullptr, "Could not get PC Singleton.", result, registry, stackId);
        auto pcCell = pc->GetParentCell();
        ERROR_AND_RETURN_EXPR_IF(pcCell == nullptr, "Could not get cell of PC.", result, registry, stackId);
        result.reserve(pcCell->GetRuntimeData().references.size());
        for (const auto& ref : pcCell->GetRuntimeData().references) {
            RE::TESObjectREFR* objectRefPtr = ref.get();
            auto actorCastedPtr = skyrim_cast<RE::Actor*>(objectRefPtr);
            if (actorCastedPtr)
                result.push_back(actorCastedPtr);
        }
        result.shrink_to_fit();
        return result;
    }

    //
    namespace ArmorFormSearchUtils {
        static struct {
            std::vector<std::string> names;
            std::vector<RE::TESObjectARMO*> armors;
            //
            void setup(std::string nameFilter, bool mustBePlayable) {
                LogExit exitPrint("ArmorFormSearchUtils.setup"sv);
                auto data = RE::TESDataHandler::GetSingleton();
                auto& list = data->GetFormArray(RE::FormType::Armor);
                const auto size = list.size();
                this->names.reserve(size);
                this->armors.reserve(size);
                for (std::uint32_t i = 0; i < size; i++) {
                    const auto form = list[i];
                    if (form && form->formType == RE::FormType::Armor) {
                        auto armor = skyrim_cast<RE::TESObjectARMO*>(form);
                        if (!armor) continue;
                        if (armor->templateArmor)// filter out predefined enchanted variants, to declutter the list
                            continue;
                        if (mustBePlayable && !!(armor->formFlags & RE::TESObjectARMO::RecordFlags::kNonPlayable))
                            continue;
                        std::string armorName;
                        {// get name
                            auto tfn = skyrim_cast<RE::TESFullName*>(armor);
                            if (tfn)
                                armorName = tfn->fullName.data();
                        }
                        if (armorName.empty())// skip nameless armor
                            continue;
                        if (!nameFilter.empty()) {
                            auto it = std::search(
                                armorName.begin(), armorName.end(),
                                nameFilter.begin(), nameFilter.end(),
                                [](char a, char b) { return toupper(a) == toupper(b); });
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
        void Prep(RE::BSScript::IVirtualMachine* registry,
                  std::uint32_t stackId,
                  RE::StaticFunctionTag*,
                  RE::BSFixedString filter,
                  bool mustBePlayable) {
            LogExit exitPrint("ArmorFormSearchUtils.Prep"sv);
            data.setup(filter.data(), mustBePlayable);
        }
        std::vector<RE::TESObjectARMO*> GetForms(RE::BSScript::IVirtualMachine* registry,
                                                 std::uint32_t stackId,
                                                 RE::StaticFunctionTag*) {
            LogExit exitPrint("ArmorFormSearchUtils.GetForms"sv);
            std::vector<RE::TESObjectARMO*> result;
            auto& list = data.armors;
            for (auto it = list.begin(); it != list.end(); it++)
                result.push_back(*it);
            std::vector<RE::TESObjectARMO*> converted_result;
            converted_result.reserve(result.size());
            for (const auto ptr : result) {
                converted_result.push_back((RE::TESObjectARMO*) ptr);
            }
            return converted_result;
        }
        std::vector<RE::BSFixedString> GetNames(RE::BSScript::IVirtualMachine* registry,
                                                std::uint32_t stackId,
                                                RE::StaticFunctionTag*) {
            LogExit exitPrint("ArmorFormSearchUtils.GetNames"sv);
            std::vector<RE::BSFixedString> result;
            auto& list = data.names;
            for (auto it = list.begin(); it != list.end(); it++)
                result.push_back(it->c_str());
            return result;
        }
        void Clear(RE::BSScript::IVirtualMachine* registry, std::uint32_t stackId, RE::StaticFunctionTag*) {
            LogExit exitPrint("ArmorFormSearchUtils.Clear"sv);
            data.clear();
        }
    }// namespace ArmorFormSearchUtils
    namespace BodySlotListing {
        enum {
            kBodySlotMin = 30,
            kBodySlotMax = 61,
        };
        static struct {
            std::vector<std::int32_t> bodySlots;
            std::vector<std::string> armorNames;
            std::vector<RE::TESObjectARMO*> armors;
        } data;
        //
        void Clear(RE::BSScript::IVirtualMachine* registry, std::uint32_t stackId, RE::StaticFunctionTag*) {
            LogExit exitPrint("BodySlotListing.Clear"sv);
            data.bodySlots.clear();
            data.armorNames.clear();
            data.armors.clear();
        }
        void Prep(RE::BSScript::IVirtualMachine* registry,
                  std::uint32_t stackId,
                  RE::StaticFunctionTag*,
                  RE::BSFixedString name) {
            LogExit exitPrint("BodySlotListing.Prep"sv);
            data.bodySlots.clear();
            data.armorNames.clear();
            data.armors.clear();
            //
            auto& service = ArmorAddonOverrideService::GetInstance();
            try {
                auto& outfit = service.getOutfit(name.data());
                auto& armors = outfit.m_armors;
                for (std::uint8_t i = kBodySlotMin; i <= kBodySlotMax; i++) {
                    std::uint32_t mask = 1 << (i - kBodySlotMin);
                    for (auto it = armors.begin(); it != armors.end(); it++) {
                        RE::TESObjectARMO* armor = *it;
                        if (armor && (static_cast<std::uint32_t>(armor->GetSlotMask()) & mask)) {
                            data.bodySlots.push_back(i);
                            data.armors.push_back(armor);
                            {// name
                                auto pFullName = skyrim_cast<RE::TESFullName*>(armor);
                                if (pFullName)
                                    data.armorNames.emplace_back(pFullName->fullName.data());
                                else
                                    data.armorNames.emplace_back("");
                            }
                        }
                    }
                }
            } catch (std::out_of_range) {
                registry->TraceStack("The specified outfit does not exist.", stackId, RE::BSScript::IVirtualMachine::Severity::kWarning);
            }
        }
        std::vector<RE::TESObjectARMO*> GetArmorForms(RE::BSScript::IVirtualMachine* registry,
                                                      std::uint32_t stackId,
                                                      RE::StaticFunctionTag*) {
            LogExit exitPrint("BodySlotListing.GetArmorForms"sv);
            std::vector<RE::TESObjectARMO*> result;
            auto& list = data.armors;
            for (auto it = list.begin(); it != list.end(); it++)
                result.push_back(*it);
            std::vector<RE::TESObjectARMO*> converted_result;
            converted_result.reserve(result.size());
            for (const auto ptr : result) {
                converted_result.push_back((RE::TESObjectARMO*) ptr);
            }
            return converted_result;
        }
        std::vector<RE::BSFixedString> GetArmorNames(RE::BSScript::IVirtualMachine* registry,
                                                     std::uint32_t stackId,
                                                     RE::StaticFunctionTag*) {
            LogExit exitPrint("BodySlotListing.GetArmorNames"sv);
            std::vector<RE::BSFixedString> result;
            auto& list = data.armorNames;
            for (auto it = list.begin(); it != list.end(); it++)
                result.push_back(it->c_str());
            return result;
        }
        std::vector<std::int32_t> GetSlotIndices(RE::BSScript::IVirtualMachine* registry,
                                                 std::uint32_t stackId,
                                                 RE::StaticFunctionTag*) {
            LogExit exitPrint("BodySlotListing.GetSlotIndices"sv);
            std::vector<std::int32_t> result;
            auto& list = data.bodySlots;
            for (auto it = list.begin(); it != list.end(); it++)
                result.push_back(*it);
            return result;
        }
    }// namespace BodySlotListing
    namespace BodySlotPolicy {
        std::vector<RE::BSFixedString> BodySlotPolicyNamesForOutfit(RE::BSScript::IVirtualMachine* registry,
                                                                    std::uint32_t stackId,
                                                                    RE::StaticFunctionTag*,
                                                                    RE::BSFixedString name) {
            LogExit exitPrint("BodySlotPolicy.BodySlotPolicyNamesForOutfit"sv);
            std::vector<RE::BSFixedString> result;
            auto& service = ArmorAddonOverrideService::GetInstance();
            auto& outfit = service.getOutfit(name.data());
            for (auto slot = RE::BIPED_OBJECTS_META::kFirstSlot; slot < RE::BIPED_OBJECTS_META::kNumSlots; slot++) {
                auto slotSpecificPolicy = outfit.m_slotPolicies.find(static_cast<RE::BIPED_OBJECT>(slot));
                if (slotSpecificPolicy != outfit.m_slotPolicies.end()) {
                    result.emplace_back(SlotPolicy::g_policiesMetadata.at(static_cast<char>(slotSpecificPolicy->second)).translationKey());
                } else {
                    result.emplace_back("$SkyOutSys_Desc_PolicyName_INHERIT");
                }
            }
            // Add in the overall outfit's policy to the end
            result.emplace_back(SlotPolicy::g_policiesMetadata.at(static_cast<char>(outfit.m_blanketSlotPolicy)).translationKey());
            return result;
        }
        void SetBodySlotPoliciesForOutfit(RE::BSScript::IVirtualMachine* registry,
                                          std::uint32_t stackId,
                                          RE::StaticFunctionTag*,
                                          RE::BSFixedString name,
                                          std::uint32_t slot,
                                          RE::BSFixedString code) {
            LogExit exitPrint("BodySlotPolicy.SetBodySlotPoliciesForOutfit"sv);
            auto& service = ArmorAddonOverrideService::GetInstance();
            auto& outfit = service.getOutfit(name.data());
            if (slot >= RE::BIPED_OBJECTS_META::kNumSlots) {
                LOG(err, "Invalid slot {}.", static_cast<std::uint32_t>(slot));
                return;
            }
            if (code.empty()) {
                outfit.setSlotPolicy(static_cast<RE::BIPED_OBJECT>(slot), std::nullopt);
            } else {
                std::string codeString(code);
                auto found = std::find_if(SlotPolicy::g_policiesMetadata.begin(), SlotPolicy::g_policiesMetadata.end(), [&](const SlotPolicy::Metadata& first) {
                    return first.code == codeString;
                });
                if (found == SlotPolicy::g_policiesMetadata.end()) return;
                outfit.setSlotPolicy(static_cast<RE::BIPED_OBJECT>(slot), static_cast<SlotPolicy::Mode>(found - SlotPolicy::g_policiesMetadata.begin()));
            }
        }
        void SetAllBodySlotPoliciesForOutfit(RE::BSScript::IVirtualMachine* registry,
                                             std::uint32_t stackId,
                                             RE::StaticFunctionTag*,
                                             RE::BSFixedString name,
                                             RE::BSFixedString code) {
            LogExit exitPrint("BodySlotPolicy.SetAllBodySlotPoliciesForOutfit"sv);
            auto& service = ArmorAddonOverrideService::GetInstance();
            auto& outfit = service.getOutfit(name.data());
            std::string codeString(code);
            auto found = std::find_if(SlotPolicy::g_policiesMetadata.begin(), SlotPolicy::g_policiesMetadata.end(), [&](const SlotPolicy::Metadata& first) {
                return first.code == codeString;
            });
            if (found == SlotPolicy::g_policiesMetadata.end()) return;
            outfit.setBlanketSlotPolicy(static_cast<SlotPolicy::Mode>(found - SlotPolicy::g_policiesMetadata.begin()));
        }
        void SetBodySlotPolicyToDefaultForOutfit(RE::BSScript::IVirtualMachine* registry,
                                                 std::uint32_t stackId,
                                                 RE::StaticFunctionTag*,
                                                 RE::BSFixedString name) {
            LogExit exitPrint("BodySlotPolicy.SetBodySlotPolicyToDefaultForOutfit"sv);
            auto& service = ArmorAddonOverrideService::GetInstance();
            auto& outfit = service.getOutfit(name.data());
            outfit.setDefaultSlotPolicy();
        }
        std::vector<RE::BSFixedString> GetAvailablePolicyNames(RE::BSScript::IVirtualMachine* registry,
                                                               std::uint32_t stackId,
                                                               RE::StaticFunctionTag*) {
            LogExit exitPrint("BodySlotPolicy.GetAvailablePolicyNames"sv);
            auto policies = std::vector<SlotPolicy::Metadata>(SlotPolicy::g_policiesMetadata.begin(), SlotPolicy::g_policiesMetadata.end());
            std::erase_if(policies, [](const SlotPolicy::Metadata& first) { return first.advanced; });
            std::sort(policies.begin(), policies.end(), [](const SlotPolicy::Metadata& first, const SlotPolicy::Metadata& second) {
                return first.sortOrder < second.sortOrder;
            });
            std::vector<RE::BSFixedString> result;
            for (const auto& policy : policies) {
                result.emplace_back(policy.translationKey());
            }
            return result;
        }
        std::vector<RE::BSFixedString> GetAvailablePolicyCodes(RE::BSScript::IVirtualMachine* registry,
                                                               std::uint32_t stackId,
                                                               RE::StaticFunctionTag*) {
            LogExit exitPrint("BodySlotPolicy.GetAvailablePolicyCodes"sv);
            auto policies = std::vector<SlotPolicy::Metadata>(SlotPolicy::g_policiesMetadata.begin(), SlotPolicy::g_policiesMetadata.end());
            std::erase_if(policies, [](const SlotPolicy::Metadata& first) { return first.advanced; });
            std::sort(policies.begin(), policies.end(), [](const SlotPolicy::Metadata& first, const SlotPolicy::Metadata& second) {
                return first.sortOrder < second.sortOrder;
            });
            std::vector<RE::BSFixedString> result;
            for (const auto& policy : policies) {
                result.emplace_back(policy.code);
            }
            return result;
        }
    }// namespace BodySlotPolicy
    namespace StringSorts {
        std::vector<RE::BSFixedString> NaturalSort_ASCII(RE::BSScript::IVirtualMachine* registry,
                                                         std::uint32_t stackId,
                                                         RE::StaticFunctionTag*,
                                                         std::vector<RE::BSFixedString> arr,
                                                         bool descending) {
            LogExit exitPrint("StringSorts.NaturalSort_ASCII"sv);
            std::vector<RE::BSFixedString> result = arr;
            std::sort(
                result.begin(),
                result.end(),
                [descending](const RE::BSFixedString& x, const RE::BSFixedString& y) {
                    std::string a(x.data());
                    std::string b(y.data());
                    if (descending)
                        std::swap(a, b);
                    return cobb::utf8::naturalcompare(a, b) > 0;
                });
            return result;
        }

        template<typename T>
        std::vector<T*> NaturalSortPair_ASCII(
            RE::BSScript::IVirtualMachine* registry,
            std::uint32_t stackId,
            RE::StaticFunctionTag*,
            std::vector<RE::BSFixedString> arr,// Array of string
            std::vector<T*> second,            // Array of forms (T)
            bool descending) {
            LogExit exitPrint("StringSorts.NaturalSortPair_ASCII"sv);
            std::size_t size = arr.size();
            if (size != second.size()) {
                registry->TraceStack("The two arrays must be the same length.", stackId, RE::BSScript::IVirtualMachine::Severity::kError);
                return second;
            }
            //
            typedef std::pair<RE::BSFixedString, T*> _pair;
            std::vector<_pair> pairs;
            //
            std::vector<RE::BSFixedString> result;
            {// Copy input array into output array
                result.reserve(size);
                for (std::uint32_t i = 0; i < size; i++) {
                    pairs.emplace_back(arr[i], second[i]);
                }
            }
            std::sort(
                pairs.begin(),
                pairs.end(),
                [descending](const _pair& x, const _pair& y) {
                    std::string a(x.first.data());
                    std::string b(y.first.data());
                    if (descending)
                        std::swap(a, b);
                    return cobb::utf8::naturalcompare(a, b) > 0;
                });
            for (std::uint32_t i = 0; i < size; i++) {
                result.push_back(pairs[i].first);
                second[i] = pairs[i].second;
            }
            return second;
        }
    }// namespace StringSorts
    namespace Utility {
        std::uint32_t HexToInt32(RE::BSScript::IVirtualMachine* registry,
                                 std::uint32_t stackId,
                                 RE::StaticFunctionTag*,
                                 RE::BSFixedString str) {
            LogExit exitPrint("Utility.HexToInt32"sv);
            const char* s = str.data();
            char* discard;
            return strtoul(s, &discard, 16);
        }
        RE::BSFixedString ToHex(RE::BSScript::IVirtualMachine* registry, std::uint32_t stackId, RE::StaticFunctionTag*,

                                std::uint32_t value,
                                std::int32_t length) {
            LogExit exitPrint("Utility.ToHex"sv);
            if (length < 1) {
                registry->TraceStack(
                    "Cannot format a hexadecimal valueinteger to a negative number of digits. Defaulting to eight.",
                    stackId, RE::BSScript::IVirtualMachine::Severity::kWarning);
                length = 8;
            } else if (length > 8) {
                registry->TraceStack("Cannot format a hexadecimal integer longer than eight digits.", stackId, RE::BSScript::IVirtualMachine::Severity::kWarning);
                length = 8;
            }
            char hex[9];
            memset(hex, '0', sizeof(hex));
            hex[length] = '\0';
            while (value > 0 && length--) {
                std::uint8_t digit = value % 0x10;
                value /= 0x10;
                if (digit < 0xA) {
                    hex[length] = digit + '0';
                } else {
                    hex[length] = digit + 0x37;
                }
            }
            return hex;// passes through RE::BSFixedString constructor, which I believe caches the string, so returning local vars should be fine
        }
    }// namespace Utility
    //
    void AddArmorToOutfit(RE::BSScript::IVirtualMachine* registry, std::uint32_t stackId, RE::StaticFunctionTag*,

                          RE::BSFixedString name,
                          RE::TESObjectARMO* armor_skse) {
        LogExit exitPrint("AddArmorToOutfit"sv);
        auto armor = (RE::TESObjectARMO*) (armor_skse);
        ERROR_AND_RETURN_IF(armor == nullptr, "Cannot add a None armor to an outfit.", registry, stackId);
        auto& service = ArmorAddonOverrideService::GetInstance();
        try {
            auto& outfit = service.getOutfit(name.data());
            outfit.m_armors.insert(armor);
        } catch (std::out_of_range) {
            registry->TraceStack("The specified outfit does not exist.", stackId, RE::BSScript::IVirtualMachine::Severity::kWarning);
        }
    }
    bool ArmorConflictsWithOutfit(RE::BSScript::IVirtualMachine* registry,
                                  std::uint32_t stackId,
                                  RE::StaticFunctionTag*,

                                  RE::TESObjectARMO* armor_skse,
                                  RE::BSFixedString name) {
        LogExit exitPrint("ArmorConflictsWithOutfit"sv);
        auto armor = (RE::TESObjectARMO*) (armor_skse);
        if (armor == nullptr) {
            registry->TraceStack("A None armor can't conflict with anything in an outfit.", stackId, RE::BSScript::IVirtualMachine::Severity::kWarning);
            return false;
        }
        auto& service = ArmorAddonOverrideService::GetInstance();
        try {
            auto& outfit = service.getOutfit(name.data());
            return outfit.conflictsWith(armor);
        } catch (std::out_of_range) {
            registry->TraceStack("The specified outfit does not exist.", stackId, RE::BSScript::IVirtualMachine::Severity::kWarning);
            return false;
        }
    }
    void CreateOutfit(RE::BSScript::IVirtualMachine* registry,
                      std::uint32_t stackId,
                      RE::StaticFunctionTag*,
                      RE::BSFixedString name) {
        LogExit exitPrint("CreateOutfit"sv);
        auto& service = ArmorAddonOverrideService::GetInstance();
        try {
            service.addOutfit(name.data());
        } catch (ArmorAddonOverrideService::bad_name) {
            registry->TraceStack("Invalid outfit name specified.", stackId, RE::BSScript::IVirtualMachine::Severity::kError);
            return;
        }
    }
    void DeleteOutfit(RE::BSScript::IVirtualMachine* registry,
                      std::uint32_t stackId,
                      RE::StaticFunctionTag*,
                      RE::BSFixedString name) {
        LogExit exitPrint("DeleteOutfit"sv);
        auto& service = ArmorAddonOverrideService::GetInstance();
        service.deleteOutfit(name.data());
    }
    std::vector<RE::TESObjectARMO*> GetOutfitContents(RE::BSScript::IVirtualMachine* registry,
                                                      std::uint32_t stackId,
                                                      RE::StaticFunctionTag*,

                                                      RE::BSFixedString name) {
        LogExit exitPrint("GetOutfitContents"sv);
        std::vector<RE::TESObjectARMO*> result;
        auto& service = ArmorAddonOverrideService::GetInstance();
        try {
            auto& outfit = service.getOutfit(name.data());
            auto& armors = outfit.m_armors;
            for (auto it = armors.begin(); it != armors.end(); ++it)
                result.push_back(*it);
        } catch (std::out_of_range) {
            registry->TraceStack("The specified outfit does not exist.", stackId, RE::BSScript::IVirtualMachine::Severity::kWarning);
        }
        std::vector<RE::TESObjectARMO*> converted_result;
        converted_result.reserve(result.size());
        for (const auto ptr : result) {
            converted_result.push_back((RE::TESObjectARMO*) ptr);
        }
        return converted_result;
    }
    bool GetOutfitFavoriteStatus(RE::BSScript::IVirtualMachine* registry,
                                 std::uint32_t stackId,
                                 RE::StaticFunctionTag*,

                                 RE::BSFixedString name) {
        LogExit exitPrint("GetOutfitFavoriteStatus"sv);
        auto& service = ArmorAddonOverrideService::GetInstance();
        bool result = false;
        try {
            auto& outfit = service.getOutfit(name.data());
            result = outfit.m_favorited;
        } catch (std::out_of_range) {
            registry->TraceStack("The specified outfit does not exist.", stackId, RE::BSScript::IVirtualMachine::Severity::kWarning);
        }
        return result;
    }
    RE::BSFixedString GetSelectedOutfit(RE::BSScript::IVirtualMachine* registry,
                                        std::uint32_t stackId,
                                        RE::StaticFunctionTag*,
                                        RE::Actor* actor) {
        LogExit exitPrint("GetSelectedOutfit"sv);
        if (!actor)
            return RE::BSFixedString("");
        auto& service = ArmorAddonOverrideService::GetInstance();
        return service.currentOutfit(actor->GetHandle().native_handle()).m_name.c_str();
    }
    bool IsEnabled(RE::BSScript::IVirtualMachine* registry, std::uint32_t stackId, RE::StaticFunctionTag*) {
        LogExit exitPrint("IsEnabled"sv);
        auto& service = ArmorAddonOverrideService::GetInstance();
        return service.enabled;
    }
    std::vector<RE::BSFixedString> ListOutfits(RE::BSScript::IVirtualMachine* registry,
                                               std::uint32_t stackId,
                                               RE::StaticFunctionTag*,

                                               bool favoritesOnly) {
        LogExit exitPrint("ListOutfits"sv);
        auto& service = ArmorAddonOverrideService::GetInstance();
        std::vector<RE::BSFixedString> result;
        std::vector<std::string> intermediate;
        service.getOutfitNames(intermediate, favoritesOnly);
        result.reserve(intermediate.size());
        for (auto it = intermediate.begin(); it != intermediate.end(); ++it)
            result.push_back(it->c_str());
        return result;
    }
    void RemoveArmorFromOutfit(RE::BSScript::IVirtualMachine* registry, std::uint32_t stackId, RE::StaticFunctionTag*,

                               RE::BSFixedString name,
                               RE::TESObjectARMO* armor_skse) {
        LogExit exitPrint("RemoveArmorFromOutfit"sv);
        auto armor = (RE::TESObjectARMO*) (armor_skse);
        ERROR_AND_RETURN_IF(armor == nullptr, "Cannot remove a None armor from an outfit.", registry, stackId);
        auto& service = ArmorAddonOverrideService::GetInstance();
        try {
            auto& outfit = service.getOutfit(name.data());
            outfit.m_armors.erase(armor);
        } catch (std::out_of_range) {
            registry->TraceStack("The specified outfit does not exist.", stackId, RE::BSScript::IVirtualMachine::Severity::kWarning);
        }
    }
    void RemoveConflictingArmorsFrom(RE::BSScript::IVirtualMachine* registry,
                                     std::uint32_t stackId,
                                     RE::StaticFunctionTag*,

                                     RE::TESObjectARMO* armor_skse,
                                     RE::BSFixedString name) {
        LogExit exitPrint("RemoveConflictingArmorsFrom"sv);
        auto armor = (RE::TESObjectARMO*) (armor_skse);
        ERROR_AND_RETURN_IF(armor == nullptr,
                            "A None armor can't conflict with anything in an outfit.",
                            registry,
                            stackId);
        auto& service = ArmorAddonOverrideService::GetInstance();
        try {
            auto& outfit = service.getOutfit(name.data());
            auto& armors = outfit.m_armors;
            std::vector<RE::TESObjectARMO*> conflicts;
            const auto candidateMask = armor->GetSlotMask();
            for (auto it = armors.begin(); it != armors.end(); ++it) {
                RE::TESObjectARMO* existing = *it;
                if (existing) {
                    const auto mask = existing->GetSlotMask();
                    if ((static_cast<uint32_t>(mask) & static_cast<uint32_t>(candidateMask))
                        != static_cast<uint32_t>(RE::BGSBipedObjectForm::FirstPersonFlag::kNone))
                        conflicts.push_back(existing);
                }
            }
            for (auto it = conflicts.begin(); it != conflicts.end(); ++it)
                armors.erase(*it);
        } catch (std::out_of_range) {
            registry->TraceStack("The specified outfit does not exist.", stackId, RE::BSScript::IVirtualMachine::Severity::kError);
            return;
        }
    }
    bool RenameOutfit(RE::BSScript::IVirtualMachine* registry, std::uint32_t stackId, RE::StaticFunctionTag*,

                      RE::BSFixedString name,
                      RE::BSFixedString changeTo) {
        LogExit exitPrint("RenameOutfit"sv);
        auto& service = ArmorAddonOverrideService::GetInstance();
        try {
            service.renameOutfit(name.data(), changeTo.data());
        } catch (ArmorAddonOverrideService::bad_name) {
            registry->TraceStack("The desired name is invalid.", stackId, RE::BSScript::IVirtualMachine::Severity::kError);
            return false;
        } catch (ArmorAddonOverrideService::name_conflict) {
            registry->TraceStack("The desired name is taken.", stackId, RE::BSScript::IVirtualMachine::Severity::kError);
            return false;
        } catch (std::out_of_range) {
            registry->TraceStack("The specified outfit does not exist.", stackId, RE::BSScript::IVirtualMachine::Severity::kError);
            return false;
        }
        return true;
    }
    void SetOutfitFavoriteStatus(RE::BSScript::IVirtualMachine* registry,
                                 std::uint32_t stackId,
                                 RE::StaticFunctionTag*,

                                 RE::BSFixedString name,
                                 bool favorite) {
        LogExit exitPrint("SetOutfitFavoriteStatus"sv);
        auto& service = ArmorAddonOverrideService::GetInstance();
        service.setFavorite(name.data(), favorite);
    }
    bool OutfitExists(RE::BSScript::IVirtualMachine* registry, std::uint32_t stackId, RE::StaticFunctionTag*,

                      RE::BSFixedString name) {
        LogExit exitPrint("OutfitExists"sv);
        auto& service = ArmorAddonOverrideService::GetInstance();
        return service.hasOutfit(name.data());
    }
    void OverwriteOutfit(RE::BSScript::IVirtualMachine* registry,
                         std::uint32_t stackId,
                         RE::StaticFunctionTag*,
                         RE::BSFixedString name,
                         std::vector<RE::TESObjectARMO*> armors) {
        LogExit exitPrint("OverwriteOutfit"sv);
        auto& service = ArmorAddonOverrideService::GetInstance();
        try {
            auto& outfit = service.getOrCreateOutfit(name.data());
            outfit.m_armors.clear();
            auto count = armors.size();
            for (std::uint32_t i = 0; i < count; i++) {
                RE::TESObjectARMO* ptr = nullptr;
                ptr = armors.at(i);
                if (ptr)
                    outfit.m_armors.insert(ptr);
            }
        } catch (ArmorAddonOverrideService::bad_name) {
            registry->TraceStack("Invalid outfit name specified.", stackId, RE::BSScript::IVirtualMachine::Severity::kError);
            return;
        }
    }
    void SetEnabled(RE::BSScript::IVirtualMachine* registry,
                    std::uint32_t stackId,
                    RE::StaticFunctionTag*,
                    bool state) {
        LogExit exitPrint("SetEnabled"sv);
        auto& service = ArmorAddonOverrideService::GetInstance();
        service.setEnabled(state);
    }
    void SetSelectedOutfit(RE::BSScript::IVirtualMachine* registry,
                           std::uint32_t stackId,
                           RE::StaticFunctionTag*,
                           RE::Actor* actor,
                           RE::BSFixedString name) {
        LogExit exitPrint("SetSelectedOutfit"sv);
        if (!actor)
            return;
        auto& service = ArmorAddonOverrideService::GetInstance();
        service.setOutfit(name.data(), actor->GetHandle().native_handle());
    }
    void AddActor(RE::BSScript::IVirtualMachine* registry,
                  std::uint32_t stackId,
                  RE::StaticFunctionTag*,
                  RE::Actor* target) {
        LogExit exitPrint("AddActor"sv);
        auto& service = ArmorAddonOverrideService::GetInstance();
        service.addActor(target->GetHandle().native_handle());
    }
    void RemoveActor(RE::BSScript::IVirtualMachine* registry,
                     std::uint32_t stackId,
                     RE::StaticFunctionTag*,
                     RE::Actor* target) {
        LogExit exitPrint("RemoveActor"sv);
        auto& service = ArmorAddonOverrideService::GetInstance();
        service.removeActor(target->GetHandle().native_handle());
    }
    std::vector<RE::Actor*> ListActors(RE::BSScript::IVirtualMachine* registry,
                                       std::uint32_t stackId,
                                       RE::StaticFunctionTag*) {
        LogExit exitPrint("ListActors"sv);
        auto& service = ArmorAddonOverrideService::GetInstance();
        auto actors = service.listActors();
        std::vector<RE::Actor*> actorVec;
        for (auto& actorRef : actors) {
            auto actor = RE::Actor::LookupByHandle(actorRef);
            if (!actor) continue;
#if _DEBUG
            LOG(debug, "INNER: Actor {} has refcount {}", actor->GetDisplayFullName(), actor->QRefCount());
#endif
            if (actor->QRefCount() == 1) {
                LOG(warn, "ListActors will return an actor {} with refcount of 1. This may crash.", actor->GetDisplayFullName());
            }
            actorVec.push_back(actor.get());
        }
        std::sort(
            actorVec.begin(),
            actorVec.end(),
            [](const RE::Actor* x, const RE::Actor* y) {
                return x < y;
            });
#if _DEBUG
        for (const auto& actor : actorVec) {
            LOG(debug, "Actor {} has refcount {}", actor->GetDisplayFullName(), actor->QRefCount());
        }
#endif
        return actorVec;
    }
    void SetLocationBasedAutoSwitchEnabled(RE::BSScript::IVirtualMachine* registry,
                                           std::uint32_t stackId,
                                           RE::StaticFunctionTag*,

                                           bool value) {
        LogExit exitPrint("SetLocationBasedAutoSwitchEnabled"sv);
        ArmorAddonOverrideService::GetInstance().setLocationBasedAutoSwitchEnabled(value);
    }
    bool GetLocationBasedAutoSwitchEnabled(RE::BSScript::IVirtualMachine* registry,
                                           std::uint32_t stackId,
                                           RE::StaticFunctionTag*) {
        LogExit exitPrint("GetLocationBasedAutoSwitchEnabled"sv);
        return ArmorAddonOverrideService::GetInstance().locationBasedAutoSwitchEnabled;
    }
    std::vector<std::uint32_t> GetAutoSwitchLocationArray(RE::BSScript::IVirtualMachine* registry,
                                                          std::uint32_t stackId,
                                                          RE::StaticFunctionTag*) {
        LogExit exitPrint("GetAutoSwitchLocationArray"sv);
        std::vector<std::uint32_t> result;
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
                 LocationType::DungeonRainy}) {
            result.push_back(std::uint32_t(i));
        }
        return result;
    }
    std::optional<LocationType> identifyLocation(RE::BGSLocation* location, RE::TESWeather* weather) {
        LogExit exitPrint("identifyLocation"sv);
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
                LOG(info, "SOS: Location has Keyword %s", keyword->GetFormEditorID());
                sprintf(message, "SOS: Location has keyword %s", keyword->GetFormEditorID());
                RE::DebugNotification(message, nullptr, false);
                */
                keywords.emplace(keyword->GetFormEditorID());
            }
            location = location->parentLoc;
        }
        return service.checkLocationType(keywords, weather_flags, RE::PlayerCharacter::GetSingleton()->CreateRefHandle().native_handle());
    }
    std::uint32_t IdentifyLocationType(RE::BSScript::IVirtualMachine* registry,
                                       std::uint32_t stackId,
                                       RE::StaticFunctionTag*,

                                       RE::BGSLocation* location_skse,
                                       RE::TESWeather* weather_skse) {
        LogExit exitPrint("IdentifyLocationType"sv);
        // NOTE: Identify the location for Papyrus. In the event no location is identified, we lie to Papyrus and say "World".
        //       Therefore, Papyrus cannot assume that locations returned have an outfit assigned, at least not for "World".
        return static_cast<std::uint32_t>(identifyLocation((RE::BGSLocation*) location_skse,
                                                           (RE::TESWeather*) weather_skse)
                                              .value_or(LocationType::World));
    }
    void SetOutfitUsingLocation(RE::BSScript::IVirtualMachine* registry, std::uint32_t stackId, RE::StaticFunctionTag*,
                                RE::Actor* actor,
                                RE::BGSLocation* location_skse,
                                RE::TESWeather* weather_skse) {
        LogExit exitPrint("SetOutfitUsingLocation"sv);
        // NOTE: Location can be NULL.
        auto& service = ArmorAddonOverrideService::GetInstance();
        if (!actor)
            return;
        if (service.locationBasedAutoSwitchEnabled) {
            auto location = identifyLocation(location_skse, weather_skse);
            // Debug notifications for location classification.
            /*
            const char* locationName = locationTypeStrings[static_cast<std::uint32_t>(location)];
            char message[100];
            sprintf_s(message, "SOS: This location is a %s.", locationName);
            RE::DebugNotification(message, nullptr, false);
            */
            if (location.has_value()) {
                service.setOutfitUsingLocation(location.value(), actor->GetHandle().native_handle());
            }
        }
    }
    void SetLocationOutfit(RE::BSScript::IVirtualMachine* registry, std::uint32_t stackId, RE::StaticFunctionTag*,
                           RE::Actor* actor,
                           std::uint32_t location,
                           RE::BSFixedString name) {
        LogExit exitPrint("SetLocationOutfit"sv);
        if (!actor)
            return;
        if (strcmp(name.data(), "") == 0) {
            // Location outfit assignment is never allowed to be empty string. Use unset instead.
            return;
        }
        return ArmorAddonOverrideService::GetInstance()
            .setLocationOutfit(LocationType(location), name.data(), actor->GetHandle().native_handle());
    }
    void UnsetLocationOutfit(RE::BSScript::IVirtualMachine* registry, std::uint32_t stackId, RE::StaticFunctionTag*,
                             RE::Actor* actor,
                             std::uint32_t location) {
        LogExit exitPrint("UnsetLocationOutfit"sv);
        if (!actor)
            return;
        return ArmorAddonOverrideService::GetInstance()
            .unsetLocationOutfit(LocationType(location), actor->GetHandle().native_handle());
    }
    RE::BSFixedString GetLocationOutfit(RE::BSScript::IVirtualMachine* registry,
                                        std::uint32_t stackId,
                                        RE::StaticFunctionTag*,
                                        RE::Actor* actor,
                                        std::uint32_t location) {
        LogExit exitPrint("GetLocationOutfit"sv);
        if (!actor)
            return RE::BSFixedString("");
        auto outfit = ArmorAddonOverrideService::GetInstance()
                          .getLocationOutfit(LocationType(location), actor->GetHandle().native_handle());
        if (outfit.has_value()) {
            return RE::BSFixedString(outfit.value().c_str());
        } else {
            // Empty string means "no outfit assigned" for this location type.
            return RE::BSFixedString("");
        }
    }
    bool ExportSettings(RE::BSScript::IVirtualMachine* registry, std::uint32_t stackId, RE::StaticFunctionTag*) {
        LogExit exitPrint("ExportSettings"sv);
        std::string outputFile = GetRuntimeDirectory() + "Data\\SKSE\\Plugins\\OutfitSystemData.json";
        auto& service = ArmorAddonOverrideService::GetInstance();
        proto::OutfitSystem data = service.save();
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
    bool ImportSettings(RE::BSScript::IVirtualMachine* registry, std::uint32_t stackId, RE::StaticFunctionTag*) {
        LogExit exitPrint("ImportSettings"sv);
        std::string inputFile = GetRuntimeDirectory() + "Data\\SKSE\\Plugins\\OutfitSystemData.json";
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
        service = ArmorAddonOverrideService(data, SKSE::GetSerializationInterface());
        std::string message = "Read JSON config from " + inputFile;
        RE::DebugNotification(message.c_str(), nullptr, false);
        return true;
    }
}// namespace OutfitSystem

bool OutfitSystem::RegisterPapyrus(RE::BSScript::IVirtualMachine* registry) {
    registry->RegisterFunction("GetOutfitNameMaxLength",
                               "SkyrimOutfitSystemNativeFuncs",
                               GetOutfitNameMaxLength);
    registry->RegisterFunction(
        "GetOutfitNameMaxLength",
        "SkyrimOutfitSystemNativeFuncs",
        GetOutfitNameMaxLength,
        true);
    registry->RegisterFunction(
        "GetCarriedArmor",
        "SkyrimOutfitSystemNativeFuncs",
        GetCarriedArmor);
    registry->RegisterFunction(
        "GetWornItems",
        "SkyrimOutfitSystemNativeFuncs",
        GetWornItems);
    registry->RegisterFunction(
        "RefreshArmorFor",
        "SkyrimOutfitSystemNativeFuncs",
        RefreshArmorFor);
    registry->RegisterFunction(
        "RefreshArmorForAllConfiguredActors",
        "SkyrimOutfitSystemNativeFuncs",
        RefreshArmorForAllConfiguredActors);
    registry->RegisterFunction(
        "ActorNearPC",
        "SkyrimOutfitSystemNativeFuncs",
        ActorsNearPC);
    //
    {// armor form search utils
        registry->RegisterFunction(
            "PrepArmorSearch",
            "SkyrimOutfitSystemNativeFuncs",
            ArmorFormSearchUtils::Prep);
        registry->RegisterFunction(
            "GetArmorSearchResultForms",
            "SkyrimOutfitSystemNativeFuncs",
            ArmorFormSearchUtils::GetForms);
        registry->RegisterFunction(
            "GetArmorSearchResultNames",
            "SkyrimOutfitSystemNativeFuncs",
            ArmorFormSearchUtils::GetNames);
        registry->RegisterFunction(
            "ClearArmorSearch",
            "SkyrimOutfitSystemNativeFuncs",
            ArmorFormSearchUtils::Clear);
    }
    {// body slot data
        registry->RegisterFunction(
            "PrepOutfitBodySlotListing",
            "SkyrimOutfitSystemNativeFuncs",
            BodySlotListing::Prep);
        registry->RegisterFunction(
            "GetOutfitBodySlotListingArmorForms",
            "SkyrimOutfitSystemNativeFuncs",
            BodySlotListing::GetArmorForms);
        registry->RegisterFunction(
            "GetOutfitBodySlotListingArmorNames",
            "SkyrimOutfitSystemNativeFuncs",
            BodySlotListing::GetArmorNames);
        registry->RegisterFunction(
            "GetOutfitBodySlotListingSlotIndices",
            "SkyrimOutfitSystemNativeFuncs",
            BodySlotListing::GetSlotIndices);
        registry->RegisterFunction(
            "ClearOutfitBodySlotListing",
            "SkyrimOutfitSystemNativeFuncs",
            BodySlotListing::Clear);
    }
    {//body slot policy
        registry->RegisterFunction(
            "BodySlotPolicyNamesForOutfit",
            "SkyrimOutfitSystemNativeFuncs",
            BodySlotPolicy::BodySlotPolicyNamesForOutfit);
        registry->RegisterFunction(
            "SetBodySlotPoliciesForOutfit",
            "SkyrimOutfitSystemNativeFuncs",
            BodySlotPolicy::SetBodySlotPoliciesForOutfit);
        registry->RegisterFunction(
            "SetAllBodySlotPoliciesForOutfit",
            "SkyrimOutfitSystemNativeFuncs",
            BodySlotPolicy::SetAllBodySlotPoliciesForOutfit);
        registry->RegisterFunction(
            "SetBodySlotPolicyToDefaultForOutfit",
            "SkyrimOutfitSystemNativeFuncs",
            BodySlotPolicy::SetBodySlotPolicyToDefaultForOutfit);
        registry->RegisterFunction(
            "GetAvailablePolicyNames",
            "SkyrimOutfitSystemNativeFuncs",
            BodySlotPolicy::GetAvailablePolicyNames);
        registry->RegisterFunction(
            "GetAvailablePolicyCodes",
            "SkyrimOutfitSystemNativeFuncs",
            BodySlotPolicy::GetAvailablePolicyCodes);
    }
    {// string sorts
        registry->RegisterFunction(
            "NaturalSort_ASCII",
            "SkyrimOutfitSystemNativeFuncs",
            StringSorts::NaturalSort_ASCII,
            true);
        registry->RegisterFunction(
            "NaturalSortPairArmor_ASCII",
            "SkyrimOutfitSystemNativeFuncs",
            StringSorts::NaturalSortPair_ASCII<RE::TESObjectARMO>,
            true);
    }
    {// Utility
        registry->RegisterFunction(
            "HexToInt32",
            "SkyrimOutfitSystemNativeFuncs",
            Utility::HexToInt32,
            true);
        registry->RegisterFunction(
            "ToHex",
            "SkyrimOutfitSystemNativeFuncs",
            Utility::ToHex,
            true);
    }
    //
    registry->RegisterFunction(
        "AddArmorToOutfit",
        "SkyrimOutfitSystemNativeFuncs",
        AddArmorToOutfit);
    registry->RegisterFunction(
        "ArmorConflictsWithOutfit",
        "SkyrimOutfitSystemNativeFuncs",
        ArmorConflictsWithOutfit);
    registry->RegisterFunction(
        "CreateOutfit",
        "SkyrimOutfitSystemNativeFuncs",
        CreateOutfit);
    registry->RegisterFunction(
        "DeleteOutfit",
        "SkyrimOutfitSystemNativeFuncs",
        DeleteOutfit);
    registry->RegisterFunction(
        "GetOutfitContents",
        "SkyrimOutfitSystemNativeFuncs",
        GetOutfitContents);
    registry->RegisterFunction(
        "GetOutfitFavoriteStatus",
        "SkyrimOutfitSystemNativeFuncs",
        GetOutfitFavoriteStatus);
    registry->RegisterFunction(
        "SetOutfitFavoriteStatus",
        "SkyrimOutfitSystemNativeFuncs",
        SetOutfitFavoriteStatus);
    registry->RegisterFunction(
        "IsEnabled",
        "SkyrimOutfitSystemNativeFuncs",
        IsEnabled);
    registry->RegisterFunction(
        "GetSelectedOutfit",
        "SkyrimOutfitSystemNativeFuncs",
        GetSelectedOutfit);
    registry->RegisterFunction(
        "ListOutfits",
        "SkyrimOutfitSystemNativeFuncs",
        ListOutfits);
    registry->RegisterFunction(
        "RemoveArmorFromOutfit",
        "SkyrimOutfitSystemNativeFuncs",
        RemoveArmorFromOutfit);
    registry->RegisterFunction(
        "RemoveConflictingArmorsFrom",
        "SkyrimOutfitSystemNativeFuncs",
        RemoveConflictingArmorsFrom);
    registry->RegisterFunction(
        "RenameOutfit",
        "SkyrimOutfitSystemNativeFuncs",
        RenameOutfit);
    registry->RegisterFunction(
        "OutfitExists",
        "SkyrimOutfitSystemNativeFuncs",
        OutfitExists);
    registry->RegisterFunction(
        "OverwriteOutfit",
        "SkyrimOutfitSystemNativeFuncs",
        OverwriteOutfit);
    registry->RegisterFunction(
        "SetEnabled",
        "SkyrimOutfitSystemNativeFuncs",
        SetEnabled);
    registry->RegisterFunction(
        "SetSelectedOutfit",
        "SkyrimOutfitSystemNativeFuncs",
        SetSelectedOutfit);
    registry->RegisterFunction(
        "AddActor",
        "SkyrimOutfitSystemNativeFuncs",
        AddActor);
    registry->RegisterFunction(
        "RemoveActor",
        "SkyrimOutfitSystemNativeFuncs",
        RemoveActor);
    registry->RegisterFunction(
        "ListActors",
        "SkyrimOutfitSystemNativeFuncs",
        ListActors);
    registry->RegisterFunction(
        "SetLocationBasedAutoSwitchEnabled",
        "SkyrimOutfitSystemNativeFuncs",
        SetLocationBasedAutoSwitchEnabled);
    registry->RegisterFunction(
        "GetLocationBasedAutoSwitchEnabled",
        "SkyrimOutfitSystemNativeFuncs",
        GetLocationBasedAutoSwitchEnabled);
    registry->RegisterFunction(
        "GetAutoSwitchLocationArray",
        "SkyrimOutfitSystemNativeFuncs",
        GetAutoSwitchLocationArray);
    registry->RegisterFunction(
        "IdentifyLocationType",
        "SkyrimOutfitSystemNativeFuncs",
        IdentifyLocationType);
    registry->RegisterFunction(
        "SetOutfitUsingLocation",
        "SkyrimOutfitSystemNativeFuncs",
        SetOutfitUsingLocation);
    registry->RegisterFunction(
        "SetLocationOutfit",
        "SkyrimOutfitSystemNativeFuncs",
        SetLocationOutfit);
    registry->RegisterFunction(
        "UnsetLocationOutfit",
        "SkyrimOutfitSystemNativeFuncs",
        UnsetLocationOutfit);
    registry->RegisterFunction(
        "GetLocationOutfit",
        "SkyrimOutfitSystemNativeFuncs",
        GetLocationOutfit);
    registry->RegisterFunction(
        "ExportSettings",
        "SkyrimOutfitSystemNativeFuncs",
        ExportSettings);
    registry->RegisterFunction(
        "ImportSettings",
        "SkyrimOutfitSystemNativeFuncs",
        ImportSettings);

    return true;
}