#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

namespace mp {

// DTO público que usa Serializer.{csv,json}
struct BlockInfo {
    void*         ptr        = nullptr;
    std::size_t   size       = 0;
    std::uint64_t alloc_id   = 0;
    std::uint32_t thread_id  = 0;
    std::uint64_t t_ns       = 0;
    std::string   callsite;            // "file:line[:func]"
};

} // namespace mp
