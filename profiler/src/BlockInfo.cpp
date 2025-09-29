#include <cstddef>
#include <cstdint>
#include <string>

namespace mp {

// DTO público que serializa Serializer.{csv,json}
struct BlockInfo {
    void*         ptr        = nullptr;      // dirección
    std::size_t   size       = 0;            // bytes
    std::uint64_t alloc_id   = 0;            // id único incremental
    std::uint32_t thread_id  = 0;            // id de hilo (hash truncado)
    std::uint64_t t_ns       = 0;            // timestamp ns (steady_clock)
    std::string   callsite;                  // "file:line[:func]" ya formateado
};

} // namespace mp
