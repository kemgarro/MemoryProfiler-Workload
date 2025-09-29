#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <thread>

#include "OperatorOverrides.hpp" // Para usar el guard reentrante en APIs que asignen internamente

namespace mp {

// Estructura solicitada
struct AllocationRecord {
    void*        ptr;
    std::size_t  size;
    const char*  type_name;      // puede ser nullptr
    std::uint64_t timestamp_ns;
    std::uint32_t thread_id;
    const char*  file;           // puede ser nullptr
    int          line;           // puede ser 0
    bool         is_array;
};

class MemoryTracker {
public:
    static MemoryTracker& instance();

    // Registros
    void onAlloc(void* p, std::size_t sz, const char* type,
                 const char* file, int line, bool isArray);

    void onFree(void* p, bool isArray) noexcept;

    // Snapshot de bloques vivos
    std::vector<AllocationRecord> snapshotLive() const;

    // Métricas
    std::size_t activeBytes() const;
    std::size_t peakBytes() const;
    std::size_t totalAllocs() const;
    std::size_t activeAllocs() const;

    // Por ahora no-op (dejado para pruebas futuras)
    void resetForTesting();

    // No copiable/movable
    MemoryTracker(const MemoryTracker&) = delete;
    MemoryTracker& operator=(const MemoryTracker&) = delete;
    MemoryTracker(MemoryTracker&&) = delete;
    MemoryTracker& operator=(MemoryTracker&&) = delete;

private:
    MemoryTracker() = default;
    ~MemoryTracker() = default;

    // Helpers
    static std::uint64_t nowNs();
    static std::uint32_t thisThreadId();

private:
    mutable std::mutex mu_;
    std::unordered_map<void*, AllocationRecord> live_;

    std::size_t total_allocs_  = 0;
    std::size_t active_allocs_ = 0;
    std::size_t total_bytes_   = 0; // (por ahora no se expone, pero se mantiene)
    std::size_t active_bytes_  = 0;
    std::size_t peak_bytes_    = 0;
};

} // namespace mp
