#pragma once
#include <functional>
#include <vector>
#include <cstdint>
#include "BlockInfo.hpp"

namespace mp {

    struct Callbacks {
        // callsite puede ser nullptr si el modelo no lo usa
        std::function<void(void*, std::size_t, const char*, const char*, int, bool)> onAlloc;
        std::function<void(void*)>                           onFree;

        std::function<std::size_t()> bytesInUse;
        std::function<std::size_t()> peakBytes;
        std::function<std::size_t()> allocCount;

        std::function<std::uint64_t()>          snapshot;
        std::function<std::vector<BlockInfo>()> liveBlocks;

        std::uint32_t version = 1;
    };

    void register_callbacks(const Callbacks& c);

    // Siempre devuelve callbacks válidos (no-op si no han registrado)
    const Callbacks& get_callbacks();

} // namespace mp