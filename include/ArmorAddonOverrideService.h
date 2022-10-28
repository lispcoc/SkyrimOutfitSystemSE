#pragma once

#include <set>
#include <unordered_map>
#include <vector>

#include "bindings.h"

namespace RE {
    class TESObjectARMO;
}

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
