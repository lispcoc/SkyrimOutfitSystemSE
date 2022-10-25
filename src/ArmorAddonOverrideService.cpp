#include "ArmorAddonOverrideService.h"

#include "RE/REAugments.h"

void _assertWrite(bool result, const char* err) {
    if (!result)
        throw ArmorAddonOverrideService::save_error(err);
}
void _assertRead(bool result, const char* err) {
    if (!result)
        throw ArmorAddonOverrideService::load_error(err);
}

Outfit::Outfit(const proto::Outfit& proto, const SKSE::SerializationInterface* intfc) {
    m_name = proto.name();
    for (const auto& formID : proto.armors()) {
        RE::FormID fixedID;
        if (intfc->ResolveFormID(formID, fixedID)) {
            auto armor = skyrim_cast<RE::TESObjectARMO*>(RE::TESForm::LookupByID(fixedID));
            if (armor)
                m_armors.insert(armor);
        }
    }
    m_favorited = proto.is_favorite();
    for (const auto& pair : proto.slot_policies()) {
        auto slot = static_cast<RE::BIPED_OBJECT>(pair.first);
        auto policy = static_cast<SlotPolicy::Mode>(pair.second);
        if (slot >= RE::BIPED_OBJECTS_META::kNumSlots) {
            LOG(err, "Invalid slot {}.", static_cast<std::uint32_t>(slot));
            continue;
        }
        if (policy >= SlotPolicy::Mode::kNumModes) {
            LOG(err, "Invalid slot preference {}", policy);
            continue;
        }
        m_slotPolicies[slot] = policy;
    }
    m_blanketSlotPolicy = static_cast<SlotPolicy::Mode>(proto.slot_policy());
}

bool Outfit::conflictsWith(RE::TESObjectARMO* test) const {
    if (!test)
        return false;
    const auto mask = static_cast<uint32_t>(test->GetSlotMask());
    for (auto it = m_armors.cbegin(); it != m_armors.cend(); ++it) {
        RE::TESObjectARMO* armor = *it;
        if (armor)
            if ((mask & static_cast<uint32_t>(armor->GetSlotMask()))
                != static_cast<uint32_t>(RE::BGSBipedObjectForm::FirstPersonFlag::kNone))
                return true;
    }
    return false;
}
bool Outfit::hasShield() const {
    auto& list = m_armors;
    for (auto it = list.cbegin(); it != list.cend(); ++it) {
        RE::TESObjectARMO* armor = *it;
        if (armor) {
            if ((armor->formFlags & RE::TESObjectARMO::RecordFlags::kShield) != 0)
                return true;
        }
    }
    return false;
};

void Outfit::setDefaultSlotPolicy() {
    m_slotPolicies.clear();
    m_slotPolicies[RE::BIPED_OBJECTS::kShield] = SlotPolicy::Mode::XEXO;
    m_blanketSlotPolicy = SlotPolicy::Mode::XXOO;
}

void Outfit::setSlotPolicy(RE::BIPED_OBJECT slot, std::optional<SlotPolicy::Mode> policy) {
    if (slot >= RE::BIPED_OBJECTS_META::kNumSlots) {
        LOG(err, "Invalid slot {}.", static_cast<std::uint32_t>(slot));
        return;
    }
    if (policy.has_value()) {
        if (policy.value() >= SlotPolicy::Mode::kNumModes) {
            LOG(err, "Invalid slot preference {}.", policy.value());
            return;
        }
        m_slotPolicies[slot] = policy.value();
    } else {
        m_slotPolicies.erase(slot);
    }
}

void Outfit::setBlanketSlotPolicy(SlotPolicy::Mode policy) {
    if (policy >= SlotPolicy::Mode::kNumModes) {
        LOG(err, "Invalid slot preference {}.", policy);
        return;
    }
    m_blanketSlotPolicy = policy;
}

std::unordered_set<RE::TESObjectARMO*> Outfit::computeDisplaySet(const std::unordered_set<RE::TESObjectARMO*>& equippedSet) {
    // Helper function that assigns the armor's to the positions in an array that match the armor's slot mask.
    auto assignToMatchingSlots = [](std::array<RE::TESObjectARMO*, RE::BIPED_OBJECTS_META::kNumSlots>& dest, RE::TESObjectARMO* armor) {
        auto mask = static_cast<uint32_t>(armor->GetSlotMask());
        for (auto slot = RE::BIPED_OBJECTS_META::kFirstSlot; slot < RE::BIPED_OBJECTS_META::kNumSlots; slot++) {
            if (mask & (1 << slot)) dest[slot] = armor;
        }
    };

    // Get the list with the equipped armor in each slot
    std::array<RE::TESObjectARMO*, RE::BIPED_OBJECTS_META::kNumSlots> equipped{nullptr};
    for (auto armor : equippedSet) {
        if (!armor) continue;
        assignToMatchingSlots(equipped, armor);
    }

    // Get the list with the outfit armor in each slot
    std::array<RE::TESObjectARMO*, RE::BIPED_OBJECTS_META::kNumSlots> outfit{nullptr};
    for (auto armor : m_armors) {
        if (!armor) continue;
        assignToMatchingSlots(outfit, armor);
    }

    // Go pairwise through the two lists and select what goes in the results slot
    std::unordered_set<RE::TESObjectARMO*> result;
    std::uint32_t occupiedMask = 0;
    for (auto slot = RE::BIPED_OBJECTS_META::kFirstSlot; slot < RE::BIPED_OBJECTS_META::kNumSlots; slot++) {
        // Someone before us already got this slot.
        if (occupiedMask & (1 << slot)) continue;
        // Select the slot's policy, falling back to the outfit's policy if none.
        SlotPolicy::Mode preference = m_blanketSlotPolicy;
        auto slotSpecificPolicy = m_slotPolicies.find(static_cast<RE::BIPED_OBJECT>(slot));
        if (slotSpecificPolicy != m_slotPolicies.end()) preference = slotSpecificPolicy->second;
        auto selection = SlotPolicy::select(preference, equipped[slot], outfit[slot]);
        RE::TESObjectARMO* selectedArmor = nullptr;
        switch (selection) {
        case SlotPolicy::Selection::EQUIPPED:
            selectedArmor = equipped[slot];
            break;
        case SlotPolicy::Selection::OUTFIT:
            selectedArmor = outfit[slot];
            break;
        case SlotPolicy::Selection::EMPTY:
            break;
        }
        if (!selectedArmor) continue;
        occupiedMask |= static_cast<uint32_t>(selectedArmor->GetSlotMask());
        result.emplace(selectedArmor);
    }

    return result;
};

proto::Outfit Outfit::save() const {
    proto::Outfit out;
    out.set_name(m_name);
    for (const auto& armor : m_armors) {
        if (armor)
            out.add_armors(armor->formID);
    }
    out.set_is_favorite(m_favorited);
    for (const auto& policy : m_slotPolicies) {
        out.mutable_slot_policies()->emplace(static_cast<std::uint32_t>(policy.first), static_cast<std::uint32_t>(policy.second));
    }
    out.set_slot_policy(static_cast<std::uint32_t>(m_blanketSlotPolicy));
    return out;
}

ArmorAddonOverrideService::ArmorAddonOverrideService(const proto::OutfitSystem& data, const SKSE::SerializationInterface* intfc) {
    // Extract data from the protobuf struct.
    enabled = data.enabled();
    std::map<RE::FormID, ActorOutfitAssignments> actorOutfitAssignmentsLocal;
    for (const auto& actorAssn : data.actor_outfit_assignments()) {
        // Lookup the actor
        RE::FormID formId;
        if (!intfc->ResolveFormID(actorAssn.first, formId))
            continue;

        ActorOutfitAssignments assignments;
        assignments.currentOutfitName =
            cobb::istring(actorAssn.second.current_outfit_name().data(), actorAssn.second.current_outfit_name().size());
        for (const auto& locOutfitData : actorAssn.second.location_based_outfits()) {
            assignments.locationOutfits.emplace(LocationType(locOutfitData.first),
                                                cobb::istring(locOutfitData.second.data(),
                                                              locOutfitData.second.size()));
        }
        actorOutfitAssignmentsLocal[formId] = assignments;
    }
    actorOutfitAssignments = actorOutfitAssignmentsLocal;
    for (const auto& outfitData : data.outfits()) {
        outfits.emplace(std::piecewise_construct,
                        std::forward_as_tuple(cobb::istring(outfitData.name().data(), outfitData.name().size())),
                        std::forward_as_tuple(outfitData, intfc));
    }
    locationBasedAutoSwitchEnabled = data.location_based_auto_switch_enabled();

    // If we have the old format's data, convert it into the current mapping
    if (!data.obsolete_current_outfit_name().empty() || !data.obsolete_location_based_outfits().empty()) {
        actorOutfitAssignments[RE::PlayerCharacter::GetSingleton()->GetFormID()].currentOutfitName =
            cobb::istring(data.obsolete_current_outfit_name().data(), data.obsolete_current_outfit_name().size());
        for (const auto& locOutfitData : data.obsolete_location_based_outfits()) {
            actorOutfitAssignments[RE::PlayerCharacter::GetSingleton()->GetFormID()].locationOutfits.emplace(LocationType(locOutfitData.first),
                                                                                                                             cobb::istring(locOutfitData.second.data(), locOutfitData.second.size()));
        }
    }
}

void ArmorAddonOverrideService::_validateNameOrThrow(const char* outfitName) {
    if (strcmp(outfitName, g_noOutfitName) == 0)
        throw bad_name("Outfits can't use a blank name.");
    if (strlen(outfitName) > ce_outfitNameMaxLength)
        throw bad_name("The outfit's name is too long.");
}
//
Outfit& ArmorAddonOverrideService::getOutfit(const char* name) {
    return outfits.at(name);
}
Outfit& ArmorAddonOverrideService::getOrCreateOutfit(const char* name) {
    _validateNameOrThrow(name);
    auto created = outfits.emplace(name, name);
    if (created.second) created.first->second.setDefaultSlotPolicy();
    return created.first->second;
}
//
void ArmorAddonOverrideService::addOutfit(const char* name) {
    _validateNameOrThrow(name);
    auto created = outfits.emplace(name, name);
    if (created.second) created.first->second.setDefaultSlotPolicy();
}
void ArmorAddonOverrideService::addOutfit(const char* name, std::vector<RE::TESObjectARMO*> armors) {
    _validateNameOrThrow(name);
    auto& created = outfits.emplace(name, name).first->second;
    for (auto it = armors.begin(); it != armors.end(); ++it) {
        auto armor = *it;
        if (armor)
            created.m_armors.insert(armor);
    }
    created.setDefaultSlotPolicy();
}
Outfit& ArmorAddonOverrideService::currentOutfit(RE::FormID target) {
    if (!actorOutfitAssignments.contains(target)) return g_noOutfit;
    if (actorOutfitAssignments.at(target).currentOutfitName == g_noOutfitName) return g_noOutfit;
    auto outfit = outfits.find(actorOutfitAssignments.at(target).currentOutfitName);
    if (outfit == outfits.end()) return g_noOutfit;
    return outfit->second;
};
bool ArmorAddonOverrideService::hasOutfit(const char* name) const {
    return outfits.contains(name);
}
void ArmorAddonOverrideService::deleteOutfit(const char* name) {
    outfits.erase(name);
    for (auto& assn : actorOutfitAssignments) {
        if (assn.second.currentOutfitName == name)
            assn.second.currentOutfitName = g_noOutfitName;
        // If the outfit is assigned as a location outfit, remove it there as well.
        for (auto it = assn.second.locationOutfits.begin(); it != assn.second.locationOutfits.end(); ++it) {
            if (it->second == name) {
                assn.second.locationOutfits.erase(it);
                break;
            }
        }
    }
}

void ArmorAddonOverrideService::setFavorite(const char* name, bool favorite) {
    auto outfit = outfits.find(name);
    if (outfit != outfits.end())
        outfit->second.m_favorited = favorite;
}

void ArmorAddonOverrideService::modifyOutfit(const char* name,
                                             std::vector<RE::TESObjectARMO*>& add,
                                             std::vector<RE::TESObjectARMO*>& remove,
                                             bool createIfMissing) {
    try {
        Outfit& target = getOutfit(name);
        for (auto it = add.begin(); it != add.end(); ++it) {
            auto armor = *it;
            if (armor)
                target.m_armors.insert(armor);
        }
        for (auto it = remove.begin(); it != remove.end(); ++it) {
            auto armor = *it;
            if (armor)
                target.m_armors.erase(armor);
        }
    } catch (std::out_of_range) {
        if (createIfMissing) {
            addOutfit(name);
            modifyOutfit(name, add, remove);
        }
    }
}
void ArmorAddonOverrideService::renameOutfit(const char* oldName, const char* newName) {
    _validateNameOrThrow(newName);
    if (outfits.contains(newName)) throw name_conflict("");
    auto outfitNode = outfits.extract(oldName);
    if (outfitNode.empty()) throw std::out_of_range("");
    outfitNode.key() = newName;
    outfitNode.mapped().m_name = newName;
    outfits.insert(std::move(outfitNode));
    for (auto& assignment : actorOutfitAssignments) {
        if (assignment.second.currentOutfitName == oldName)
            assignment.second.currentOutfitName = newName;
        // If the outfit is assigned as a location outfit, remove it there as well.
        for (auto& locationOutfit : assignment.second.locationOutfits) {
            if (locationOutfit.second == oldName) {
                assignment.second.locationOutfits[locationOutfit.first] = newName;
                break;
            }
        }
    }
}
void ArmorAddonOverrideService::setOutfit(const char* name, RE::FormID target) {
    if (!actorOutfitAssignments.contains(target)) return;
    if (strcmp(name, g_noOutfitName) == 0) {
        actorOutfitAssignments.at(target).currentOutfitName = g_noOutfitName;
        return;
    }
    try {
        getOutfit(name);
        actorOutfitAssignments.at(target).currentOutfitName = name;
    } catch (std::out_of_range) {
        LOG(info,
            "ArmorAddonOverrideService: Tried to set non-existent outfit %s as active. Switching the system off for now.",
            name);
        actorOutfitAssignments.at(target).currentOutfitName = g_noOutfitName;
    }
}

void ArmorAddonOverrideService::addActor(RE::FormID target) {
    if (actorOutfitAssignments.count(target) == 0)
        actorOutfitAssignments.emplace(target, ActorOutfitAssignments());
}

void ArmorAddonOverrideService::removeActor(RE::FormID target) {
    if (target == RE::PlayerCharacter::GetSingleton()->GetFormID())
        return;
    actorOutfitAssignments.erase(target);
}

std::unordered_set<RE::FormID> ArmorAddonOverrideService::listActors() {
    std::unordered_set<RE::FormID> actors;
    for (auto& assignment : actorOutfitAssignments) {
        actors.insert(assignment.first);
    }
    return actors;
}

void ArmorAddonOverrideService::setLocationBasedAutoSwitchEnabled(bool newValue) noexcept {
    locationBasedAutoSwitchEnabled = newValue;
}

void ArmorAddonOverrideService::setOutfitUsingLocation(LocationType location, RE::FormID target) {
    if (actorOutfitAssignments.count(target) == 0)
        return;
    auto it = actorOutfitAssignments.at(target).locationOutfits.find(location);
    if (it != actorOutfitAssignments.at(target).locationOutfits.end()) {
        setOutfit(it->second.c_str(), target);
    }
}

void ArmorAddonOverrideService::setLocationOutfit(LocationType location, const char* name, RE::FormID target) {
    if (actorOutfitAssignments.count(target) == 0)
        return;
    if (!std::string(name).empty()) {// Can never set outfit to the "" outfit. Use unsetLocationOutfit instead.
        actorOutfitAssignments.at(target).locationOutfits[location] = name;
    }
}

void ArmorAddonOverrideService::unsetLocationOutfit(LocationType location, RE::FormID target) {
    if (actorOutfitAssignments.count(target) == 0)
        return;
    actorOutfitAssignments.at(target).locationOutfits.erase(location);
}

std::optional<cobb::istring> ArmorAddonOverrideService::getLocationOutfit(LocationType location, RE::FormID target) {
    if (actorOutfitAssignments.count(target) == 0)
        return std::optional<cobb::istring>();
    ;
    auto it = actorOutfitAssignments.at(target).locationOutfits.find(location);
    if (it != actorOutfitAssignments.at(target).locationOutfits.end()) {
        return std::optional<cobb::istring>(it->second);
    } else {
        return std::optional<cobb::istring>();
    }
}

#define CHECK_LOCATION(TYPE, CHECK_CODE)                                                             \
    if (actorOutfitAssignments.at(target).locationOutfits.count(LocationType::TYPE) && (CHECK_CODE)) \
        return std::optional<LocationType>(LocationType::TYPE);

std::optional<LocationType> ArmorAddonOverrideService::checkLocationType(const std::unordered_set<std::string>& keywords,
                                                                         const WeatherFlags& weather_flags,
                                                                         RE::FormID target) {
    if (actorOutfitAssignments.count(target) == 0)
        return std::optional<LocationType>();

    CHECK_LOCATION(CitySnowy, keywords.count("LocTypeCity") && weather_flags.snowy);
    CHECK_LOCATION(CityRainy, keywords.count("LocTypeCity") && weather_flags.rainy);
    CHECK_LOCATION(City, keywords.count("LocTypeCity"));

    // A city is considered a town, so it will use the town outfit unless a city one is selected.
    CHECK_LOCATION(TownSnowy, keywords.count("LocTypeTown") + keywords.count("LocTypeCity") && weather_flags.snowy);
    CHECK_LOCATION(TownRainy, keywords.count("LocTypeTown") + keywords.count("LocTypeCity") && weather_flags.rainy);
    CHECK_LOCATION(Town, keywords.count("LocTypeTown") + keywords.count("LocTypeCity"));

    CHECK_LOCATION(DungeonSnowy, keywords.count("LocTypeDungeon") && weather_flags.snowy);
    CHECK_LOCATION(DungeonRainy, keywords.count("LocTypeDungeon") && weather_flags.rainy);
    CHECK_LOCATION(Dungeon, keywords.count("LocTypeDungeon"));

    CHECK_LOCATION(WorldSnowy, weather_flags.snowy);
    CHECK_LOCATION(WorldRainy, weather_flags.rainy);
    CHECK_LOCATION(World, true);

    return std::optional<LocationType>();
}

bool ArmorAddonOverrideService::shouldOverride(RE::FormID target) const noexcept {
    if (!enabled)
        return false;
    if (actorOutfitAssignments.count(target) == 0)
        return false;
    if (actorOutfitAssignments.at(target).currentOutfitName == g_noOutfitName)
        return false;
    return true;
}
void ArmorAddonOverrideService::getOutfitNames(std::vector<std::string>& out, bool favoritesOnly) const {
    out.clear();
    auto& list = outfits;
    out.reserve(list.size());
    for (auto it = list.cbegin(); it != list.cend(); ++it)
        if (!favoritesOnly || it->second.m_favorited)
            out.push_back(it->second.m_name);
}

void ArmorAddonOverrideService::setEnabled(bool flag) noexcept {
    enabled = flag;
}

proto::OutfitSystem ArmorAddonOverrideService::save() {
    proto::OutfitSystem out;
    out.set_enabled(enabled);
    for (const auto& actorAssn : actorOutfitAssignments) {
        // Store a reference to the actor
        RE::FormID formId;
        formId = actorAssn.first;

        proto::ActorOutfitAssignment assnOut;
        assnOut.set_current_outfit_name(actorAssn.second.currentOutfitName.data(),
                                        actorAssn.second.currentOutfitName.size());
        for (const auto& lbo : actorAssn.second.locationOutfits) {
            assnOut.mutable_location_based_outfits()
                ->insert({static_cast<std::uint32_t>(lbo.first), std::string(lbo.second.data(), lbo.second.size())});
        }
        out.mutable_actor_outfit_assignments()->insert({formId, assnOut});
    }
    for (const auto& outfit : outfits) {
        auto newOutfit = out.add_outfits();
        *newOutfit = outfit.second.save();
    }
    out.set_location_based_auto_switch_enabled(locationBasedAutoSwitchEnabled);
    return out;
}
//
void ArmorAddonOverrideService::dump() const {
    LOG(info, "Dumping all state for ArmorAddonOverrideService...");
    LOG(info, "Enabled: %d", enabled);
    LOG(info, "We have %d outfits. Enumerating...", outfits.size());
    for (auto it = outfits.begin(); it != outfits.end(); ++it) {
        LOG(info, " - Key: %s", it->first.c_str());
        LOG(info, "    - Name: %s", it->second.m_name.c_str());
        LOG(info, "    - Armors:");
        auto& list = it->second.m_armors;
        for (auto jt = list.begin(); jt != list.end(); ++jt) {
            auto ptr = *jt;
            if (ptr) {
                LOG(info, "       - (TESObjectARMO*){} == [ARMO:{}]", (void*) ptr, ptr->formID);
            } else {
                LOG(info, "       - nullptr");
            }
        }
    }
    LOG(info, "All state has been dumped.");
}

void ArmorAddonOverrideService::migrateSaveVersionV5() {
    LOG(info, "Migrating outfit slot settings");
    for (auto& outfit : outfits) {
        outfit.second.setDefaultSlotPolicy();
    }
}

void ArmorAddonOverrideService::migrateSaveVersionV6() {
    LOG(info, "Migrating actors to FormID");
    actorOutfitAssignments.clear();
}

void ArmorAddonOverrideService::checkConsistency() {
    if (!actorOutfitAssignments.contains(RE::PlayerCharacter::GetSingleton()->GetFormID())) {
        actorOutfitAssignments[RE::PlayerCharacter::GetSingleton()->GetFormID()] = ActorOutfitAssignments();
    }
}

namespace SlotPolicy {
    // Negative values mean "advanced option"
    std::array<Metadata, kNumModes> g_policiesMetadata = {
        Metadata{"XXXX", 100, true},// Never show anything
        Metadata{"XXXE", 101, true},// If outfit and equipped, show equipped
        Metadata{"XXXO", 2, false}, // If outfit and equipped, show outfit (require equipped, no passthrough)
        Metadata{"XXOX", 102, true},// If only outfit, show outfit
        Metadata{"XXOE", 103, true},// If only outfit, show outfit. If both, show equipped
        Metadata{"XXOO", 1, false}, // If outfit, show outfit (always show outfit, no passthough)
        Metadata{"XEXX", 104, true},// If only equipped, show equipped
        Metadata{"XEXE", 105, true},// If equipped, show equipped
        Metadata{"XEXO", 3, false}, // If only equipped, show equipped. If both, show outfit
        Metadata{"XEOX", 106, true},// If only equipped, show equipped. If only outfit, show outfit
        Metadata{"XEOE", 107, true},// If only equipped, show equipped. If only outfit, show outfit. If both, show equipped
        Metadata{"XEOO", 108, true} // If only equipped, show equipped. If only outfit, show outfit. If both, show outfit
    };

    Selection select(Mode policy, bool hasEquipped, bool hasOutfit) {
        if (policy >= Mode::kNumModes) {
            LOG(err, "Invalid slot preference {}", policy);
            policy = Mode::XXXX;
        }
        char out;
        if (!hasEquipped && !hasOutfit) {
            out = g_policiesMetadata[policy].code[0];
        } else if (hasEquipped && !hasOutfit) {
            out = g_policiesMetadata[policy].code[1];
        } else if (!hasEquipped && hasOutfit) {
            out = g_policiesMetadata[policy].code[2];
        } else if (hasEquipped && hasOutfit) {
            out = g_policiesMetadata[policy].code[3];
        }
        switch (out) {
        case 'E':
            return Selection::EQUIPPED;
        case 'O':
            return Selection::OUTFIT;
        default:
            return Selection::EMPTY;
        }
    }
}// namespace SlotPolicy
