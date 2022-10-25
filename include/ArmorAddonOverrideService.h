#pragma once

#include <set>
#include <unordered_map>
#include <vector>

#include "cobb/strings.h"

#include "bindings.h"

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
    enum Mode : std::uint8_t {
        XXXX,
        XXXE,
        XXXO,
        XXOX,
        XXOE,
        XXOO,
        XEXX,
        XEXE,
        XEXO,
        XEOX,
        XEOE,
        XEOO,
        kNumModes
    };

    struct Metadata {
        std::string code;
        std::int32_t sortOrder;
        bool advanced;
        std::string translationKey() const {
            return "$SkyOutSys_Desc_EasyPolicyName_" + code;
        }
    };

    extern std::array<Metadata, kNumModes> g_policiesMetadata;

    enum class Selection {
        EMPTY,
        EQUIPPED,
        OUTFIT
    };

    Selection select(Mode policy, bool hasEquipped, bool hasOutfit);
}// namespace SlotPolicy

OutfitService& GetRustInstance();

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

namespace Peristence {
    static constexpr std::uint32_t signature = 'AAOS';
    enum {
        kSaveVersionV1 = 1,// Unsupported handwritten binary format
        kSaveVersionV2 = 2,// Unsupported handwritten binary format
        kSaveVersionV3 = 3,// Unsupported handwritten binary format
        kSaveVersionV4 = 4,// First version with protobuf
        kSaveVersionV5 = 5,// First version with Slot Control System
        kSaveVersionV6 = 6,// Switch to FormID for Actor (instead of Handle)
    };
}
