#pragma once

namespace RE {
    namespace BSScript {
        class IVirtualMachine;
    }
}// namespace RE

namespace OutfitSystem {
    bool RegisterPapyrus(RE::BSScript::IVirtualMachine* registry);
}
