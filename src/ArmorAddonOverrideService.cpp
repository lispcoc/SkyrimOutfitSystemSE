#include "ArmorAddonOverrideService.h"

void _assertWrite(bool result, const char* err) {
    if (!result)
        throw ArmorAddonOverrideService::save_error(err);
}
void _assertRead(bool result, const char* err) {
    if (!result)
        throw ArmorAddonOverrideService::load_error(err);
}

bool Outfit::conflictsWith(RE::TESObjectARMO* test) const {
    if (!test)
        return false;
    const auto mask = static_cast<uint32_t>(test->GetSlotMask());
    for (auto it = this->armors.cbegin(); it != this->armors.cend(); ++it) {
        RE::TESObjectARMO* armor = *it;
        if (armor)
            if ((mask & static_cast<uint32_t>(armor->GetSlotMask()))
                != static_cast<uint32_t>(RE::BGSBipedObjectForm::FirstPersonFlag::kNone))
                return true;
    }
    return false;
}
bool Outfit::hasShield() const {
    auto& list = this->armors;
    for (auto it = list.cbegin(); it != list.cend(); ++it) {
        RE::TESObjectARMO* armor = *it;
        if (armor) {
            if ((armor->formFlags & RE::TESObjectARMO::RecordFlags::kShield) != 0)
                return true;
        }
    }
    return false;
};

SlotPolicy::Preference Outfit::effectivePolicyForSlot(RE::BIPED_OBJECT slot) {
    auto policy = slotPolicies.find(slot);
    if (policy != slotPolicies.end()) return policy->second;
    return slotPolicy;
}

void Outfit::setDefaultSlotPolicy() {
    slotPolicies.clear();
    slotPolicies[RE::BIPED_OBJECTS::kShield] = SlotPolicy::Preference::XEXO;
    slotPolicy = SlotPolicy::Preference::XXOO;
}

void Outfit::setAllSlotPolicy(SlotPolicy::Preference preference) {
    if (preference < SlotPolicy::Preference::XXXX || preference >= SlotPolicy::Preference::MAX) {
        LOG(err, "Invalid slot preference {}.", static_cast<char>(preference));
        return;
    }
    slotPolicy = preference;
}

void Outfit::setSlotPolicy(RE::BIPED_OBJECT slot, std::optional<SlotPolicy::Preference> policy) {
    if (slot >= SlotPolicy::numSlots) {
        LOG(err, "Invalid slot {}.", static_cast<std::uint32_t>(slot));
        return;
    }
    if (policy.has_value()) {
        if (policy.value() < SlotPolicy::Preference::XXXX || policy.value() >= SlotPolicy::Preference::MAX) {
            LOG(err, "Invalid slot preference {}.", static_cast<char>(policy.value()));
            return;
        }
        slotPolicies[slot] = policy.value();
    } else {
        slotPolicies.erase(slot);
    }
}

std::unordered_set<RE::TESObjectARMO*> Outfit::computeDisplaySet(const std::unordered_set<RE::TESObjectARMO*>& equippedSet) {
    std::unordered_set<RE::TESObjectARMO*> result;

    std::array<RE::TESObjectARMO*, SlotPolicy::numSlots> equipped{nullptr};
    std::array<RE::TESObjectARMO*, SlotPolicy::numSlots> outfit{nullptr};
    std::uint32_t occupiedMask = 0;

    for (auto armor : equippedSet) {
        if (!armor) continue;
        auto mask = static_cast<uint32_t>(armor->GetSlotMask());
        for (auto slot = SlotPolicy::firstSlot; slot < SlotPolicy::numSlots; slot++) {
            if (mask & (1 << slot)) equipped[slot] = armor;
        }
    }

    for (auto armor : armors) {
        if (!armor) continue;
        auto mask = static_cast<uint32_t>(armor->GetSlotMask());
        for (auto slot = SlotPolicy::firstSlot; slot < SlotPolicy::numSlots; slot++) {
            if (mask & (1 << slot)) outfit[slot] = armor;
        }
    }

    for (auto slot = SlotPolicy::firstSlot; slot < SlotPolicy::numSlots; slot++) {
        // Someone before us already got this slot.
        if (occupiedMask & (1 << slot)) continue;
        // Select the slot's policy, falling back to the outfit's policy if none.
        SlotPolicy::Preference preference = slotPolicy;
        auto slotSpecificPolicy = slotPolicies.find(static_cast<RE::BIPED_OBJECT>(slot));
        if (slotSpecificPolicy != slotPolicies.end()) preference = slotSpecificPolicy->second;
        auto selection = SlotPolicy::select(preference, equipped[slot], outfit[slot]);
        RE::TESObjectARMO* selectedArmor = nullptr;
        switch (selection) {
        case SlotPolicy::Selection::EMPTY:
            break;
        case SlotPolicy::Selection::EQUIPPED:
            selectedArmor = equipped[slot];
            break;
        case SlotPolicy::Selection::OUTFIT:
            selectedArmor = outfit[slot];
            break;
        }
        if (!selectedArmor) continue;
        occupiedMask |= static_cast<uint32_t>(selectedArmor->GetSlotMask());
        result.emplace(selectedArmor);
    }

    return result;
};

void Outfit::load_legacy(const SKSE::SerializationInterface* intfc, std::uint32_t version) {
    std::uint32_t size = 0;
    _assertRead(intfc->ReadRecordData(size), "Failed to read an outfit's armor count.");
    for (std::uint32_t i = 0; i < size; i++) {
        RE::FormID formID = 0;
        _assertRead(intfc->ReadRecordData(formID), "Failed to read an outfit's armor.");
        RE::FormID fixedID;
        if (intfc->ResolveFormID(formID, fixedID)) {
            auto armor = skyrim_cast<RE::TESObjectARMO*>(RE::TESForm::LookupByID(fixedID));
            if (armor)
                this->armors.insert(armor);
        }
    }
    if (version >= ArmorAddonOverrideService::kSaveVersionV1) {
        _assertRead(intfc->ReadRecordData(isFavorite), "Failed to read an outfit's favorite status.");
    } else {
        this->isFavorite = false;
    }
    setDefaultSlotPolicy();
}

void Outfit::load(const proto::Outfit& proto, const SKSE::SerializationInterface* intfc) {
    this->name = proto.name();
    for (const auto& formID : proto.armors()) {
        RE::FormID fixedID;
        if (intfc->ResolveFormID(formID, fixedID)) {
            auto armor = skyrim_cast<RE::TESObjectARMO*>(RE::TESForm::LookupByID(fixedID));
            if (armor)
                this->armors.insert(armor);
        }
    }
    this->isFavorite = proto.is_favorite();
    for (const auto& pair : proto.slot_policies()) {
        auto slot = static_cast<RE::BIPED_OBJECT>(pair.first);
        auto policy = static_cast<SlotPolicy::Preference>(pair.second);
        if (slot >= SlotPolicy::numSlots) {
            LOG(err, "Invalid slot {}.", static_cast<std::uint32_t>(slot));
            continue;
        }
        if (policy < SlotPolicy::Preference::XXXX || policy >= SlotPolicy::Preference::MAX) {
            LOG(err, "Invalid slot preference {}", static_cast<char>(policy));
            continue;
        }
        slotPolicies[slot] = policy;
    }
    slotPolicy = static_cast<SlotPolicy::Preference>(proto.slot_policy());
}

proto::Outfit Outfit::save() const {
    proto::Outfit out;
    out.set_name(this->name);
    for (const auto& armor : this->armors) {
        if (armor)
            out.add_armors(armor->formID);
    }
    out.set_is_favorite(this->isFavorite);
    for (const auto& policy : slotPolicies) {
        out.mutable_slot_policies()->emplace(static_cast<std::uint32_t>(policy.first), static_cast<std::uint32_t>(policy.second));
    }
    out.set_slot_policy(static_cast<std::uint32_t>(slotPolicy));
    return out;
}

void ArmorAddonOverrideService::_validateNameOrThrow(const char* outfitName) {
    if (strcmp(outfitName, g_noOutfitName) == 0)
        throw bad_name("Outfits can't use a blank name.");
    if (strlen(outfitName) > ce_outfitNameMaxLength)
        throw bad_name("The outfit's name is too long.");
}
//
Outfit& ArmorAddonOverrideService::getOutfit(const char* name) {
    return this->outfits.at(name);
}
Outfit& ArmorAddonOverrideService::getOrCreateOutfit(const char* name) {
    _validateNameOrThrow(name);
    auto created = this->outfits.emplace(name, name);
    if (created.second) created.first->second.setDefaultSlotPolicy();
    return created.first->second;
}
//
void ArmorAddonOverrideService::addOutfit(const char* name) {
    _validateNameOrThrow(name);
    auto created = this->outfits.emplace(name, name);
    if (created.second) created.first->second.setDefaultSlotPolicy();
}
void ArmorAddonOverrideService::addOutfit(const char* name, std::vector<RE::TESObjectARMO*> armors) {
    _validateNameOrThrow(name);
    auto& created = this->outfits.emplace(name, name).first->second;
    for (auto it = armors.begin(); it != armors.end(); ++it) {
        auto armor = *it;
        if (armor)
            created.armors.insert(armor);
    }
    created.setDefaultSlotPolicy();
}
Outfit& ArmorAddonOverrideService::currentOutfit(RE::RawActorHandle target) {
    if (this->actorOutfitAssignments.count(target) == 0)
        return g_noOutfit;
    if (this->actorOutfitAssignments.at(target).currentOutfitName == g_noOutfitName)
        return g_noOutfit;
    try {
        return outfits.at(actorOutfitAssignments.at(target).currentOutfitName);
    } catch (std::out_of_range) {
        return g_noOutfit;
    }
};
bool ArmorAddonOverrideService::hasOutfit(const char* name) const {
    try {
        static_cast<void>(this->outfits.at(name));
        return true;
    } catch (std::out_of_range) {
        return false;
    }
}
void ArmorAddonOverrideService::deleteOutfit(const char* name) {
    this->outfits.erase(name);
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
    auto outfit = this->outfits.find(name);
    if (outfit != this->outfits.end())
        outfit->second.isFavorite = favorite;
}

void ArmorAddonOverrideService::modifyOutfit(const char* name,
                                             std::vector<RE::TESObjectARMO*>& add,
                                             std::vector<RE::TESObjectARMO*>& remove,
                                             bool createIfMissing) {
    try {
        Outfit& target = this->getOutfit(name);
        for (auto it = add.begin(); it != add.end(); ++it) {
            auto armor = *it;
            if (armor)
                target.armors.insert(armor);
        }
        for (auto it = remove.begin(); it != remove.end(); ++it) {
            auto armor = *it;
            if (armor)
                target.armors.erase(armor);
        }
    } catch (std::out_of_range) {
        if (createIfMissing) {
            this->addOutfit(name);
            this->modifyOutfit(name, add, remove);
        }
    }
}
void ArmorAddonOverrideService::renameOutfit(const char* oldName, const char* newName) {
    _validateNameOrThrow(newName);
    try {
        static_cast<void>(this->outfits.at(newName));
        throw name_conflict("");
    } catch (std::out_of_range) {
        Outfit& renamed = (this->outfits[newName] = this->outfits
                                                        .at(oldName));// don't try-catch this "at" call; let the caller catch the exception
        renamed.name = newName;
        this->outfits.erase(oldName);
        for (auto& assn : actorOutfitAssignments) {
            if (assn.second.currentOutfitName == oldName)
                assn.second.currentOutfitName = newName;
            // If the outfit is assigned as a location outfit, remove it there as well.
            for (auto& locationOutfit : assn.second.locationOutfits) {
                if (locationOutfit.second == oldName) {
                    assn.second.locationOutfits[locationOutfit.first] = newName;
                    break;
                }
            }
        }
    }
}
void ArmorAddonOverrideService::setOutfit(const char* name, RE::RawActorHandle target) {
    if (this->actorOutfitAssignments.count(target) == 0)
        return;
    if (strcmp(name, g_noOutfitName) == 0) {
        this->actorOutfitAssignments.at(target).currentOutfitName = g_noOutfitName;
        return;
    }
    try {
        this->getOutfit(name);
        this->actorOutfitAssignments.at(target).currentOutfitName = name;
    } catch (std::out_of_range) {
        LOG(info,
            "ArmorAddonOverrideService: Tried to set non-existent outfit %s as active. Switching the system off for now.",
            name);
        this->actorOutfitAssignments.at(target).currentOutfitName = g_noOutfitName;
    }
}

void ArmorAddonOverrideService::addActor(RE::RawActorHandle target) {
    if (actorOutfitAssignments.count(target) == 0)
        actorOutfitAssignments.emplace(target, ActorOutfitAssignments());
}

void ArmorAddonOverrideService::removeActor(RE::RawActorHandle target) {
    if (target == RE::PlayerCharacter::GetSingleton()->GetHandle().native_handle())
        return;
    actorOutfitAssignments.erase(target);
}

std::unordered_set<RE::RawActorHandle> ArmorAddonOverrideService::listActors() {
    std::unordered_set<RE::RawActorHandle> actors;
    for (auto& assignment : actorOutfitAssignments) {
        actors.insert(assignment.first);
    }
    return actors;
}

void ArmorAddonOverrideService::setLocationBasedAutoSwitchEnabled(bool newValue) noexcept {
    locationBasedAutoSwitchEnabled = newValue;
}

void ArmorAddonOverrideService::setOutfitUsingLocation(LocationType location, RE::RawActorHandle target) {
    if (this->actorOutfitAssignments.count(target) == 0)
        return;
    auto it = this->actorOutfitAssignments.at(target).locationOutfits.find(location);
    if (it != this->actorOutfitAssignments.at(target).locationOutfits.end()) {
        this->setOutfit(it->second.c_str(), target);
    }
}

void ArmorAddonOverrideService::setLocationOutfit(LocationType location, const char* name, RE::RawActorHandle target) {
    if (this->actorOutfitAssignments.count(target) == 0)
        return;
    if (!std::string(name).empty()) {// Can never set outfit to the "" outfit. Use unsetLocationOutfit instead.
        this->actorOutfitAssignments.at(target).locationOutfits[location] = name;
    }
}

void ArmorAddonOverrideService::unsetLocationOutfit(LocationType location, RE::RawActorHandle target) {
    if (this->actorOutfitAssignments.count(target) == 0)
        return;
    this->actorOutfitAssignments.at(target).locationOutfits.erase(location);
}

std::optional<cobb::istring> ArmorAddonOverrideService::getLocationOutfit(LocationType location, RE::RawActorHandle target) {
    if (this->actorOutfitAssignments.count(target) == 0)
        return std::optional<cobb::istring>();
    ;
    auto it = this->actorOutfitAssignments.at(target).locationOutfits.find(location);
    if (it != this->actorOutfitAssignments.at(target).locationOutfits.end()) {
        return std::optional<cobb::istring>(it->second);
    } else {
        return std::optional<cobb::istring>();
    }
}

#define CHECK_LOCATION(TYPE, CHECK_CODE)                                                                   \
    if (this->actorOutfitAssignments.at(target).locationOutfits.count(LocationType::TYPE) && (CHECK_CODE)) \
        return std::optional<LocationType>(LocationType::TYPE);

std::optional<LocationType> ArmorAddonOverrideService::checkLocationType(const std::unordered_set<std::string>& keywords,
                                                                         const WeatherFlags& weather_flags,
                                                                         RE::RawActorHandle target) {
    if (this->actorOutfitAssignments.count(target) == 0)
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

bool ArmorAddonOverrideService::shouldOverride(RE::RawActorHandle target) const noexcept {
    if (!this->enabled)
        return false;
    if (this->actorOutfitAssignments.count(target) == 0)
        return false;
    if (this->actorOutfitAssignments.at(target).currentOutfitName == g_noOutfitName)
        return false;
    return true;
}
void ArmorAddonOverrideService::getOutfitNames(std::vector<std::string>& out, bool favoritesOnly) const {
    out.clear();
    auto& list = this->outfits;
    out.reserve(list.size());
    for (auto it = list.cbegin(); it != list.cend(); ++it)
        if (!favoritesOnly || it->second.isFavorite)
            out.push_back(it->second.name);
}
void ArmorAddonOverrideService::setEnabled(bool flag) noexcept {
    this->enabled = flag;
}
//
void ArmorAddonOverrideService::reset() {
    this->enabled = true;
    this->actorOutfitAssignments.clear();
    this->actorOutfitAssignments[RE::PlayerCharacter::GetSingleton()->GetHandle().native_handle()] = ActorOutfitAssignments();
    this->outfits.clear();
    this->locationBasedAutoSwitchEnabled = false;
}

void ArmorAddonOverrideService::load_legacy(const SKSE::SerializationInterface* intfc, std::uint32_t version) {
    this->reset();
    //
    std::string selectedOutfitName;
    _assertWrite(intfc->ReadRecordData(this->enabled), "Failed to read the enable state.");
    {// current outfit name
        //
        // we can't call WriteData directly on this->currentOutfitName because it's
        // a cobb::istring, and SKSE only templated WriteData for std::string in
        // specific; other basic_string classes break it.
        //
        std::uint32_t size = 0;
        char buf[257];
        memset(buf, '\0', sizeof(buf));
        _assertRead(intfc->ReadRecordData(size), "Failed to read the selected outfit name.");
        _assertRead(size < 257, "The selected outfit name is too long.");
        if (size) {
            _assertRead(intfc->ReadRecordData(buf, size), "Failed to read the selected outfit name.");
        }
        selectedOutfitName = buf;
    }
    std::uint32_t size;
    _assertRead(intfc->ReadRecordData(size), "Failed to read the outfit count.");
    for (std::uint32_t i = 0; i < size; i++) {
        std::string name;
        _assertRead(intfc->ReadRecordData(name), "Failed to read an outfit's name.");
        auto& outfit = this->getOrCreateOutfit(name.c_str());
        outfit.load_legacy(intfc, version);
    }
    this->setOutfit(selectedOutfitName.c_str(), RE::PlayerCharacter::GetSingleton()->GetHandle().native_handle());
    if (version >= ArmorAddonOverrideService::kSaveVersionV3) {
        _assertWrite(intfc->ReadRecordData(this->locationBasedAutoSwitchEnabled),
                     "Failed to read the autoswitch enable state.");
        std::uint32_t autoswitchSize =
            static_cast<std::uint32_t>(this->actorOutfitAssignments.at(RE::PlayerCharacter::GetSingleton()->GetHandle().native_handle())
                                           .locationOutfits.size());
        _assertRead(intfc->ReadRecordData(autoswitchSize), "Failed to read the number of autoswitch slots.");
        for (std::uint32_t i = 0; i < autoswitchSize; i++) {
            // get location outfit
            //
            // we can't call WriteData directly on this->currentOutfitName because it's
            // a cobb::istring, and SKSE only templated WriteData for std::string in
            // specific; other basic_string classes break it.
            //
            LocationType autoswitchSlot;
            _assertRead(intfc->ReadRecordData(autoswitchSlot), "Failed to read the an autoswitch slot ID.");
            std::uint32_t locationOutfitNameSize = 0;
            char locationOutfitName[257];
            memset(locationOutfitName, '\0', sizeof(locationOutfitName));
            _assertRead(intfc->ReadRecordData(locationOutfitNameSize), "Failed to read the an autoswitch outfit name.");
            _assertRead(locationOutfitNameSize < 257, "The autoswitch outfit name is too long.");
            if (locationOutfitNameSize) {
                _assertRead(intfc->ReadRecordData(locationOutfitName, locationOutfitNameSize),
                            "Failed to read the selected outfit name.");
                this->actorOutfitAssignments.at(RE::PlayerCharacter::GetSingleton()->GetHandle().native_handle()).locationOutfits.emplace(autoswitchSlot, locationOutfitName);
            }
        }
    } else {
        this->actorOutfitAssignments.at(RE::PlayerCharacter::GetSingleton()->GetHandle().native_handle()).locationOutfits =
            std::map<LocationType, cobb::istring>();
    }
}

void ArmorAddonOverrideService::load(const SKSE::SerializationInterface* intfc, const proto::OutfitSystem& data) {
    this->reset();
    // Extract data from the protobuf struct.
    this->enabled = data.enabled();
    std::map<RE::RawActorHandle, ActorOutfitAssignments> actorOutfitAssignmentsLocal;
    for (const auto& actorAssn : data.actor_outfit_assignments()) {
        // Lookup the actor
        std::uint64_t handle;
        RE::NiPointer<RE::Actor> actor;
        if (!intfc->ResolveHandle(actorAssn.first, handle))
            continue;

        ActorOutfitAssignments assignments;
        assignments.currentOutfitName =
            cobb::istring(actorAssn.second.current_outfit_name().data(), actorAssn.second.current_outfit_name().size());
        for (const auto& locOutfitData : actorAssn.second.location_based_outfits()) {
            assignments.locationOutfits.emplace(LocationType(locOutfitData.first),
                                                cobb::istring(locOutfitData.second.data(),
                                                              locOutfitData.second.size()));
        }
        actorOutfitAssignmentsLocal[static_cast<RE::RawActorHandle>(handle)] = assignments;
    }
    this->actorOutfitAssignments = actorOutfitAssignmentsLocal;
    for (const auto& outfitData : data.outfits()) {
        Outfit outfit;
        outfit.load(outfitData, intfc);
        this->outfits.emplace(cobb::istring(outfitData.name().data(), outfitData.name().size()), outfit);
    }
    this->locationBasedAutoSwitchEnabled = data.location_based_auto_switch_enabled();

    // If the loaded actor assignments are empty, then we know we loaded the old format... convert from that instead.
    if (actorOutfitAssignmentsLocal.empty()) {
        this->actorOutfitAssignments[RE::PlayerCharacter::GetSingleton()->GetHandle().native_handle()].currentOutfitName =
            cobb::istring(data.obsolete_current_outfit_name().data(), data.obsolete_current_outfit_name().size());
        for (const auto& locOutfitData : data.obsolete_location_based_outfits()) {
            this->actorOutfitAssignments[RE::PlayerCharacter::GetSingleton()->GetHandle().native_handle()].locationOutfits.emplace(LocationType(locOutfitData.first),
                                                                                                                                   cobb::istring(locOutfitData.second.data(), locOutfitData.second.size()));
        }
    }
}

proto::OutfitSystem ArmorAddonOverrideService::save() {
    proto::OutfitSystem out;
    out.set_enabled(this->enabled);
    for (const auto& actorAssn : actorOutfitAssignments) {
        // Store a reference to the actor
        std::uint64_t handle;
        handle = actorAssn.first;

        proto::ActorOutfitAssignment assnOut;
        assnOut.set_current_outfit_name(actorAssn.second.currentOutfitName.data(),
                                        actorAssn.second.currentOutfitName.size());
        for (const auto& lbo : actorAssn.second.locationOutfits) {
            assnOut.mutable_location_based_outfits()
                ->insert({static_cast<std::uint32_t>(lbo.first), std::string(lbo.second.data(), lbo.second.size())});
        }
        out.mutable_actor_outfit_assignments()->insert({handle, assnOut});
    }
    for (const auto& outfit : this->outfits) {
        auto newOutfit = out.add_outfits();
        *newOutfit = outfit.second.save();
    }
    out.set_location_based_auto_switch_enabled(this->locationBasedAutoSwitchEnabled);
    return out;
}
//
void ArmorAddonOverrideService::dump() const {
    LOG(info, "Dumping all state for ArmorAddonOverrideService...");
    LOG(info, "Enabled: %d", this->enabled);
    LOG(info, "We have %d outfits. Enumerating...", this->outfits.size());
    for (auto it = this->outfits.begin(); it != this->outfits.end(); ++it) {
        LOG(info, " - Key: %s", it->first.c_str());
        LOG(info, "    - Name: %s", it->second.name.c_str());
        LOG(info, "    - Armors:");
        auto& list = it->second.armors;
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

// Negative values mean "advanced option"
std::array<SlotPolicy::Metadata, static_cast<char>(SlotPolicy::Preference::MAX)> SlotPolicy::g_policiesMetadata = {
    SlotPolicy::Metadata{"XXXX", 100, true},   // Never show anything
    SlotPolicy::Metadata{"XXXE", 101, true},   // If outfit and equipped, show equipped
    SlotPolicy::Metadata{"XXXO", 2, false},    // If outfit and equipped, show outfit (require equipped, no passthrough)
    SlotPolicy::Metadata{"XXOX", 102, true},   // If only outfit, show outfit
    SlotPolicy::Metadata{"XXOE", 103, true},   // If only outfit, show outfit. If both, show equipped
    SlotPolicy::Metadata{"XXOO", 1, false},    // If outfit, show outfit (always show outfit, no passthough)
    SlotPolicy::Metadata{"XEXX", 104, true},   // If only equipped, show equipped
    SlotPolicy::Metadata{"XEXE", 105, true},   // If equipped, show equipped
    SlotPolicy::Metadata{"XEXO", 3, false},    // If only equipped, show equipped. If both, show outfit
    SlotPolicy::Metadata{"XEOX", 106, true},   // If only equipped, show equipped. If only outfit, show outfit
    SlotPolicy::Metadata{"XEOE", 107, true},   // If only equipped, show equipped. If only outfit, show outfit. If both, show equipped
    SlotPolicy::Metadata{"XEOO", 108, true}    // If only equipped, show equipped. If only outfit, show outfit. If both, show outfit
};

SlotPolicy::Selection SlotPolicy::select(SlotPolicy::Preference policy, bool hasEquipped, bool hasOutfit) {
    if (policy < Preference::XXXX || policy >= Preference::MAX) {
        LOG(err, "Invalid slot preference {}", static_cast<char>(policy));
        policy = Preference::XXXX;
    }
    char out = 'X';
    if (!hasEquipped && !hasOutfit) {
        out = g_policiesMetadata[static_cast<char>(policy)].code[0];
    } else if (hasEquipped && !hasOutfit) {
        out = g_policiesMetadata[static_cast<char>(policy)].code[1];
    } else if (!hasEquipped && hasOutfit) {
        out = g_policiesMetadata[static_cast<char>(policy)].code[2];
    } else if (hasEquipped && hasOutfit) {
        out = g_policiesMetadata[static_cast<char>(policy)].code[3];
    }
    if (out == 'X') {
        return SlotPolicy::Selection::EMPTY;
    } else if (out == 'E') {
        return SlotPolicy::Selection::EQUIPPED;
    } else if (out == 'O') {
        return SlotPolicy::Selection::OUTFIT;
    } else {
        return SlotPolicy::Selection::EMPTY;
    }
}
