#include "ArmorAddonOverrideService.h"
#pragma warning( push )
#pragma warning( disable : 5053 ) // CommonLibSSE uses explicit(<expr>) vendor extension.
#include "RE/FormComponents/TESForm/TESObject/TESBoundObject/TESObjectARMO.h"
#pragma warning( pop )
#include "skse64/GameForms.h"
#include "skse64/GameRTTI.h"
#include "skse64/Serialization.h"

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
         if ((mask & static_cast<uint32_t>(armor->GetSlotMask())) != static_cast<uint32_t>(RE::BGSBipedObjectForm::FirstPersonFlag::kNone))
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

void Outfit::load_legacy(SKSESerializationInterface* intfc, UInt32 version) {
   using namespace Serialization;
   //
   UInt32 size = 0;
   _assertRead(ReadData(intfc, &size), "Failed to read an outfit's armor count.");
   for (UInt32 i = 0; i < size; i++) {
      UInt32 formID = 0;
      _assertRead(ReadData(intfc, &formID), "Failed to read an outfit's armor.");
      UInt32 fixedID;
      if (intfc->ResolveFormId(formID, &fixedID)) {
         auto armor = reinterpret_cast<RE::TESObjectARMO*>(Runtime_DynamicCast((void*) LookupFormByID(fixedID), RTTI_TESForm, RTTI_TESObjectARMO));
         if (armor)
            this->armors.insert(armor);
      }
   }
   if (version >= ArmorAddonOverrideService::kSaveVersionV1) {
       _assertRead(ReadData(intfc, &isFavorite), "Failed to read an outfit's favorite status.");
   } else {
       this->isFavorite = false;
   }
}

void Outfit::load(const proto::Outfit& proto, SKSESerializationInterface* intfc) {
    this->name = proto.name();
    for (const auto& formID : proto.armors()) {
        UInt32 fixedID;
        if (intfc->ResolveFormId(formID, &fixedID)) {
            auto armor = reinterpret_cast<RE::TESObjectARMO*>(Runtime_DynamicCast((void*) LookupFormByID(fixedID), RTTI_TESForm, RTTI_TESObjectARMO));
            if (armor)
                this->armors.insert(armor);
        }
    }
    this->isFavorite = proto.is_favorite();
}

proto::Outfit Outfit::save(SKSESerializationInterface*) const {
   using namespace Serialization;
   //
   proto::Outfit out;
   out.set_name(this->name);
   for (const auto& armor : this->armors) {
       if (armor) out.add_armors(armor->formID);
   }
   out.set_is_favorite(this->isFavorite);
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
   return this->outfits.emplace(name, name).first->second;
}
//
void ArmorAddonOverrideService::addOutfit(const char* name) {
   _validateNameOrThrow(name);
   this->outfits.emplace(name, name);
}
void ArmorAddonOverrideService::addOutfit(const char* name, std::vector<RE::TESObjectARMO*> armors) {
   _validateNameOrThrow(name);
   auto& created = this->outfits.emplace(name, name).first->second;
   for (auto it = armors.begin(); it != armors.end(); ++it) {
      auto armor = *it;
      if (armor)
         created.armors.insert(armor);
   }
}
Outfit& ArmorAddonOverrideService::currentOutfit() {
   if (this->currentOutfitName == g_noOutfitName)
      return g_noOutfit;
   try {
      return this->outfits.at(this->currentOutfitName);
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
   if (this->currentOutfitName == name)
      this->currentOutfitName = g_noOutfitName;
   // If the outfit is assigned as a location outfit, remove it there as well.
   for (auto it = locationOutfits.begin(); it != locationOutfits.end(); ++it) {
       if (it->second == name) {
           locationOutfits.erase(it);
           break;
       }
   }
}

void ArmorAddonOverrideService::setFavorite(const char* name, bool favorite) {
   auto outfit = this->outfits.find(name);
   if (outfit != this->outfits.end())
      outfit->second.isFavorite = favorite;
}

void ArmorAddonOverrideService::modifyOutfit(const char* name, std::vector<RE::TESObjectARMO*>& add, std::vector<RE::TESObjectARMO*>& remove, bool createIfMissing) {
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
      Outfit& renamed = (this->outfits[newName] = this->outfits.at(oldName)); // don't try-catch this "at" call; let the caller catch the exception
      renamed.name = newName;
      this->outfits.erase(oldName);
      if (this->currentOutfitName == oldName)
         this->currentOutfitName = newName;
       // If the outfit is assigned as a location outfit, remove it there as well.
       for (auto & locationOutfit : locationOutfits) {
           if (locationOutfit.second == oldName) {
               locationOutfits[locationOutfit.first] = newName;
               break;
           }
       }
   }
}
void ArmorAddonOverrideService::setOutfit(const char* name) {
   if (strcmp(name, g_noOutfitName) == 0) {
      this->currentOutfitName = g_noOutfitName;
      return;
   }
   try {
      this->getOutfit(name);
      this->currentOutfitName = name;
   } catch (std::out_of_range) {
      _MESSAGE("ArmorAddonOverrideService: Tried to set non-existent outfit %s as active. Switching the system off for now.", name);
      this->currentOutfitName = g_noOutfitName;
   }
}

void ArmorAddonOverrideService::setLocationBasedAutoSwitchEnabled(bool newValue) noexcept {
    locationBasedAutoSwitchEnabled = newValue;
}

void ArmorAddonOverrideService::setOutfitUsingLocation(LocationType location) {
    auto it = locationOutfits.find(location);
    if (it != locationOutfits.end()) {
        this->setOutfit(it->second.c_str());
    }
}

void ArmorAddonOverrideService::setLocationOutfit(LocationType location, const char* name) {
    if (!std::string(name).empty()) { // Can never set outfit to the "" outfit. Use unsetLocationOutfit instead.
        locationOutfits[location] = name;
    }
}

void ArmorAddonOverrideService::unsetLocationOutfit(LocationType location) {
    locationOutfits.erase(location);
}

std::optional<cobb::istring> ArmorAddonOverrideService::getLocationOutfit(LocationType location) {
    auto it = locationOutfits.find(location);
    if (it != locationOutfits.end()) {
        return std::optional<cobb::istring>(it->second);
    } else {
        return std::optional<cobb::istring>();
    }
}

LocationType ArmorAddonOverrideService::checkLocationType(const std::unordered_set<std::string>& keywords, const WeatherFlags& weather_flags) {
    if (keywords.count("LocTypeHabitation")) {
        return LocationType::Town;
    } else if (keywords.count("LocTypeDungeon")) {
        return LocationType::Dungeon;
    }
    // By now, we know we're in no location more specific than "World"
    //
    // We will now perform weather checks and return the value *only if* that
    // weather has an outfit assigned.
    if (weather_flags.snowy && locationOutfits.count(LocationType::WorldSnowy)) {
        return LocationType::WorldSnowy;
    }
    return LocationType::World;
}

bool ArmorAddonOverrideService::shouldOverride() const noexcept {
   if (!this->enabled)
      return false;
   if (this->currentOutfitName == g_noOutfitName)
      return false;
   return true;
}
void ArmorAddonOverrideService::getOutfitNames(std::vector<std::string>& out, bool favoritesOnly) const {
   out.clear();
   auto& list = this->outfits;
   out.reserve(list.size());
   for (auto it = list.cbegin(); it != list.cend(); ++it)
      if (!favoritesOnly || it->second.isFavorite) out.push_back(it->second.name);
}
void ArmorAddonOverrideService::setEnabled(bool flag) noexcept {
   this->enabled = flag;
}
//
void ArmorAddonOverrideService::reset() {
   this->enabled = true;
   this->currentOutfitName = g_noOutfitName;
   this->outfits.clear();
   this->locationBasedAutoSwitchEnabled = false;
   this->locationOutfits.clear();
}

void ArmorAddonOverrideService::load_legacy(SKSESerializationInterface* intfc, UInt32 version) {
   using namespace Serialization;
   //
   this->reset();
   //
   std::string selectedOutfitName;
   _assertWrite(ReadData(intfc, &this->enabled),      "Failed to read the enable state.");
   {  // current outfit name
      //
      // we can't call WriteData directly on this->currentOutfitName because it's 
      // a cobb::istring, and SKSE only templated WriteData for std::string in 
      // specific; other basic_string classes break it.
      //
      UInt32 size = 0;
      char buf[257];
      memset(buf, '\0', sizeof(buf));
      _assertRead(ReadData(intfc, &size), "Failed to read the selected outfit name.");
      _assertRead(size < 257, "The selected outfit name is too long.");
      if (size) {
         _assertRead(intfc->ReadRecordData(buf, size), "Failed to read the selected outfit name.");
      }
      selectedOutfitName = buf;
   }
   UInt32 size;
   _assertRead(ReadData(intfc, &size), "Failed to read the outfit count.");
   for (UInt32 i = 0; i < size; i++) {
      std::string name;
      _assertRead(ReadData(intfc, &name), "Failed to read an outfit's name.");
      auto& outfit = this->getOrCreateOutfit(name.c_str());
       outfit.load_legacy(intfc, version);
   }
   this->setOutfit(selectedOutfitName.c_str());
   if (version >= ArmorAddonOverrideService::kSaveVersionV3) {
       _assertWrite(ReadData(intfc, &this->locationBasedAutoSwitchEnabled), "Failed to read the autoswitch enable state.");
       UInt32 autoswitchSize = static_cast<UInt32>(this->locationOutfits.size());
       _assertRead(ReadData(intfc, &autoswitchSize), "Failed to read the number of autoswitch slots.");
       for (UInt32 i = 0; i < autoswitchSize; i++) {
           // get location outfit
           //
           // we can't call WriteData directly on this->currentOutfitName because it's
           // a cobb::istring, and SKSE only templated WriteData for std::string in
           // specific; other basic_string classes break it.
           //
           LocationType autoswitchSlot;
           _assertRead(ReadData(intfc, &autoswitchSlot), "Failed to read the an autoswitch slot ID.");
           UInt32 locationOutfitNameSize = 0;
           char locationOutfitName[257];
           memset(locationOutfitName, '\0', sizeof(locationOutfitName));
           _assertRead(ReadData(intfc, &locationOutfitNameSize), "Failed to read the an autoswitch outfit name.");
           _assertRead(locationOutfitNameSize < 257, "The autoswitch outfit name is too long.");
           if (locationOutfitNameSize) {
               _assertRead(intfc->ReadRecordData(locationOutfitName, locationOutfitNameSize), "Failed to read the selected outfit name.");
               this->locationOutfits.emplace(autoswitchSlot, locationOutfitName);
           }
       }
   } else {
       this->locationOutfits = std::map<LocationType, cobb::istring>();
   }
}

void ArmorAddonOverrideService::load(SKSESerializationInterface* intfc, const proto::OutfitSystem& data) {
    this->reset();
    // Extract data from the protobuf struct.
    this->enabled = data.enabled();
    this->currentOutfitName = cobb::istring(data.current_outfit_name().data(), data.current_outfit_name().size());
    for (const auto& outfitData : data.outfits()) {
        Outfit outfit;
        outfit.load(outfitData, intfc);
        this->outfits.emplace(cobb::istring(outfitData.name().data(), outfitData.name().size()), outfit);
    }
    this->locationBasedAutoSwitchEnabled = data.location_based_auto_switch_enabled();
    for (const auto& locOutfitData : data.location_based_outfits()) {
        this->locationOutfits.emplace(LocationType(locOutfitData.first),
                cobb::istring(locOutfitData.second.data(), locOutfitData.second.size()));
    }
}

proto::OutfitSystem ArmorAddonOverrideService::save(SKSESerializationInterface* intfc) {
   using namespace Serialization;
   //
   proto::OutfitSystem out;
   out.set_enabled(this->enabled);
   out.set_current_outfit_name(this->currentOutfitName.data(), this->currentOutfitName.size());
   for (const auto& outfit : this->outfits) {
       auto newOutfit = out.add_outfits();
       *newOutfit = outfit.second.save(intfc);
   }
   out.set_location_based_auto_switch_enabled(this->locationBasedAutoSwitchEnabled);
   for (const auto& lbo : this->locationOutfits) {
       out.mutable_location_based_outfits()->insert({static_cast<UInt32>(lbo.first), std::string(lbo.second.data(), lbo.second.size())});
   }
   return out;

}
//
void ArmorAddonOverrideService::dump() const {
   _MESSAGE("Dumping all state for ArmorAddonOverrideService...");
   _MESSAGE("Enabled: %d", this->enabled);
   _MESSAGE("We have %d outfits. Enumerating...", this->outfits.size());
   for (auto it = this->outfits.begin(); it != this->outfits.end(); ++it) {
      _MESSAGE(" - Key: %s", it->first.c_str());
      _MESSAGE("    - Name: %s", it->second.name.c_str());
      _MESSAGE("    - Armors:");
      auto& list = it->second.armors;
      for (auto jt = list.begin(); jt != list.end(); ++jt) {
         auto ptr = *jt;
         if (ptr) {
            _MESSAGE("       - (TESObjectARMO*)%08X == [ARMO:%08X]", ptr, ptr->formID);
         } else {
            _MESSAGE("       - nullptr");
         }
      }
   }
   _MESSAGE("All state has been dumped.");
}
