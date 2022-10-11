#pragma once

#include <set>
#include <unordered_map>
#include <vector>

#include "cobb/strings.h"
#include "outfit.pb.h"

namespace RE {
    class TESObjectARMO;
}

enum class LocationType : std::uint32_t {
    World = 0,
    Town = 1,
    Dungeon = 2,
    City = 9,

    WorldSnowy = 3,
    TownSnowy = 4,
    DungeonSnowy = 5,
    CitySnowy = 10,

    WorldRainy = 6,
    TownRainy = 7,
    DungeonRainy = 8,
    CityRainy = 11
};

struct WeatherFlags {
    bool snowy = false;
    bool rainy = false;
};

namespace SlotPolicy {
    enum class Preference: char {
        XXXX = 0, XXXE, XXXO, XXOX, XXOE, XXOO, XEXX, XEXE, XEXO, XEOX, XEOE, XEOO, MAX
    };

    enum class Selection {
        EMPTY, EQUIPPED, OUTFIT
    };

    struct Metadata {
        std::string code;
        std::int32_t sortOrder;
        bool advanced;
        std::string translationKey() const {
            return "$SkyOutSys_Desc_PolicyName_" + code;
        }
    };

    extern std::array<Metadata, static_cast<char>(Preference::MAX)> g_policiesMetadata;
    inline constexpr Preference defaultPolicy = Preference::XXOO;

    inline constexpr std::uint32_t firstSlot = RE::BIPED_OBJECTS::kHead;
    inline constexpr std::uint32_t numSlots = RE::BIPED_OBJECTS::kEditorTotal;

    Selection select(Preference policy, bool hasEquipped, bool hasOutfit);
}

struct Outfit {
    Outfit(){};// we shouldn't need this, really, but std::unordered_map is a brat
    Outfit(const char* n) : name(n), isFavorite(false), slotPolicies{SlotPolicy::defaultPolicy} {};
    Outfit(const Outfit& other) = default;
    Outfit(const char* n, const Outfit& other) : name(n), isFavorite(false), slotPolicies{SlotPolicy::defaultPolicy} {
        this->armors = other.armors;
    }
    std::string name;// can't be const; prevents assigning to Outfit vars
    std::unordered_set<RE::TESObjectARMO*> armors;
    bool isFavorite;
    std::array<SlotPolicy::Preference, SlotPolicy::numSlots> slotPolicies;

    bool conflictsWith(RE::TESObjectARMO*) const;
    bool hasShield() const;
    std::unordered_set<RE::TESObjectARMO*> computeDisplaySet(const std::unordered_set<RE::TESObjectARMO*>& equippedSet);

    void setSlotPolicy(RE::BIPED_OBJECT slot, SlotPolicy::Preference policy);
    void setDefaultSlotPolicy();
    void setAllSlotPolicy(SlotPolicy::Preference policy);

    void load(const proto::Outfit& proto, const SKSE::SerializationInterface*);
    void load_legacy(const SKSE::SerializationInterface* intfc, std::uint32_t version);// can throw ArmorAddonOverrideService::load_error
    proto::Outfit save() const;                                                        // can throw ArmorAddonOverrideService::save_error
};
const constexpr char* g_noOutfitName = "";
static Outfit g_noOutfit(g_noOutfitName);// can't be const; prevents us from assigning it to Outfit&s

class ArmorAddonOverrideService {
public:
    typedef Outfit Outfit;
    static constexpr std::uint32_t signature = 'AAOS';
    enum {
        kSaveVersionV1 = 1,
        kSaveVersionV2 = 2,
        kSaveVersionV3 = 3,
        kSaveVersionV4 = 4// First version with protobuf
    };
    //
    static constexpr std::uint32_t ce_outfitNameMaxLength = 256;// SKSE caps serialized std::strings and const char*s to 256 bytes.
    //
    static void _validateNameOrThrow(const char* outfitName);
    //
    struct bad_name: public std::runtime_error {
        explicit bad_name(const std::string& what_arg) : runtime_error(what_arg){};
    };
    struct load_error: public std::runtime_error {
        explicit load_error(const std::string& what_arg) : runtime_error(what_arg){};
    };
    struct name_conflict: public std::runtime_error {
        explicit name_conflict(const std::string& what_arg) : runtime_error(what_arg){};
    };
    struct save_error: public std::runtime_error {
        explicit save_error(const std::string& what_arg) : runtime_error(what_arg){};
    };
    //
private:
    struct OutfitReferenceByName: public cobb::istring {
        OutfitReferenceByName(const value_type* s) : cobb::istring(s){};
        OutfitReferenceByName(const basic_string& other) : cobb::istring(other){};
        //
        operator Outfit&() {
            return ArmorAddonOverrideService::GetInstance().outfits.at(*this);
        }
    };

public:
    struct ActorOutfitAssignments {
        cobb::istring currentOutfitName = g_noOutfitName;
        std::map<LocationType, cobb::istring> locationOutfits;
    };
    bool enabled = true;
    std::map<cobb::istring, Outfit> outfits;
    // TODO: You probably shouldn't use an Actor pointer to refer to actors. It works for the PlayerCharacter, but likely not for NPCs.
    std::map<RE::RawActorHandle, ActorOutfitAssignments> actorOutfitAssignments;
    // Location-based switching
    bool locationBasedAutoSwitchEnabled = false;
    //
    static ArmorAddonOverrideService& GetInstance() {
        static ArmorAddonOverrideService instance;
        return instance;
    };
    //
    Outfit& getOutfit(const char* name);        // throws std::out_of_range if not found
    Outfit& getOrCreateOutfit(const char* name);// can throw bad_name
    //
    void addOutfit(const char* name);                                        // can throw bad_name
    void addOutfit(const char* name, std::vector<RE::TESObjectARMO*> armors);// can throw bad_name
    Outfit& currentOutfit(RE::RawActorHandle target);
    bool hasOutfit(const char* name) const;
    void deleteOutfit(const char* name);
    void setFavorite(const char* name, bool favorite);
    void setOutfitPassthrough(const char* name, bool allowPassthrough);
    void setOutfitEquipRequired(const char* name, bool requiresEquipped);
    void modifyOutfit(const char* name, std::vector<RE::TESObjectARMO*>& add, std::vector<RE::TESObjectARMO*>& remove, bool createIfMissing = false);// can throw bad_name if (createIfMissing)
    void renameOutfit(const char* oldName, const char* newName);                                                                                     // throws name_conflict if the new name is already taken; can throw bad_name; throws std::out_of_range if the oldName doesn't exist
    void setOutfit(const char* name, RE::RawActorHandle target);
    void addActor(RE::RawActorHandle target);
    void removeActor(RE::RawActorHandle target);
    std::unordered_set<RE::RawActorHandle> listActors();
    //
    void setLocationBasedAutoSwitchEnabled(bool) noexcept;
    void setOutfitUsingLocation(LocationType location, RE::RawActorHandle target);
    void setLocationOutfit(LocationType location, const char* name, RE::RawActorHandle target);
    void unsetLocationOutfit(LocationType location, RE::RawActorHandle target);
    std::optional<cobb::istring> getLocationOutfit(LocationType location, RE::RawActorHandle target);
    std::optional<LocationType> checkLocationType(const std::unordered_set<std::string>& keywords, const WeatherFlags& weather_flags, RE::RawActorHandle target);
    //
    bool shouldOverride(RE::RawActorHandle target) const noexcept;
    void getOutfitNames(std::vector<std::string>& out, bool favoritesOnly = false) const;
    void setEnabled(bool) noexcept;
    //
    void refreshCurrentIfChanged(const char* testName);
    //
    void reset();
    void load(const SKSE::SerializationInterface* intfc, const proto::OutfitSystem& data);// can throw load_error
    void load_legacy(const SKSE::SerializationInterface* intfc, std::uint32_t version);   // can throw load_error
    proto::OutfitSystem save();                                                           // can throw save_error
    //
    void dump() const;
};