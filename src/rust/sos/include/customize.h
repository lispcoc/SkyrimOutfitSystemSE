//
// Created by m on 10/23/2022.
//

#ifndef SKYRIMOUTFITSYSTEMSE_SRC_RUST_COMMONLIBSSE_SRC_CUSTOMIZE_H
#define SKYRIMOUTFITSYSTEMSE_SRC_RUST_COMMONLIBSSE_SRC_CUSTOMIZE_H

#if RUST_DEFINES
#include <cstdint>
#include <memory>
#include <string>

namespace SKSE {
    class SerializationInterface;
}

namespace RE {
    class TESObjectARMO;
    namespace BIPED_OBJECTS {
        enum BIPED_OBJECT: std::uint32_t {};
    }
    using BIPED_OBJECT = BIPED_OBJECTS::BIPED_OBJECT;
}

std::unique_ptr<std::string> GetRuntimePath();
std::unique_ptr<std::string> GetRuntimeName();
std::unique_ptr<std::string> GetRuntimeDirectory();

#endif

#endif //SKYRIMOUTFITSYSTEMSE_SRC_RUST_COMMONLIBSSE_SRC_CUSTOMIZE_H
