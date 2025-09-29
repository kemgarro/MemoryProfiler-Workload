#include "MemoryTracker.hpp"
#include <new> // std::nothrow (por si se usa en el futuro)
#include "ReentryGuard.hpp"  // para ScopedHookGuard

namespace mp {

// === Helpers estaticos ===

// Devuelve el tiempo actual en nanosegundos
uint64_t MemoryTracker::nowNs() {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

// Devuelve un identificador unico para el hilo actual (reducido a 32 bits)
uint32_t MemoryTracker::thisThreadId() {
    auto h = std::hash<std::thread::id>{}(std::this_thread::get_id());
    // Reducir a 32 bits de forma portable
    return static_cast<uint32_t>(h & 0xFFFFFFFFu);
}

// === Singleton ===

// Devuelve la unica instancia de MemoryTracker (patron singleton)
MemoryTracker& MemoryTracker::instance() {
    static MemoryTracker inst;
    return inst;
}

// === Registro de asignacion ===

// Se llama cada vez que se asigna memoria
void MemoryTracker::onAlloc(void* p, std::size_t sz, const char* type,
                            const char* file, int line, bool isArray) {
    if (!p || sz == 0) {
        // Si malloc devolvio nullptr (o size==0), no registramos
        return;
    }

    AllocationRecord rec;
    rec.ptr          = p;              // Direccion de memoria
    rec.size         = sz;             // Tamaño en bytes
    rec.type_name    = type;           // Nombre del tipo
    rec.timestamp_ns = nowNs();        // Tiempo de asignacion
    rec.thread_id    = thisThreadId(); // Id del hilo
    rec.file         = file;           // Archivo fuente
    rec.line         = line;           // Numero de linea
    rec.is_array     = isArray;        // Si fue new[] en lugar de new

    std::lock_guard<std::mutex> lock(mu_);

    // Guardamos el registro en la tabla de asignaciones vivas
    live_.emplace(p, rec);

    // Actualizamos metricas
    ++total_allocs_;
    ++active_allocs_;
    total_bytes_  += sz;
    active_bytes_ += sz;
    if (active_bytes_ > peak_bytes_) {
        peak_bytes_ = active_bytes_;
    }
}

// === Registro de liberacion ===

// Se llama cada vez que se libera memoria
void MemoryTracker::onFree(void* p, bool /*isArray*/) noexcept {
    if (!p) return;

    std::lock_guard<std::mutex> lock(mu_);

    auto it = live_.find(p);
    if (it != live_.end()) {
        const auto sz = it->second.size;
        if (active_bytes_ >= sz) active_bytes_ -= sz; // seguridad
        if (active_allocs_ > 0)  --active_allocs_;
        live_.erase(it); // eliminamos el registro
    }
    // Importante: no lanzar excepciones aqui
}

// === Snapshot de bloques vivos ===

// Devuelve una copia de todos los bloques de memoria vivos
std::vector<AllocationRecord> MemoryTracker::snapshotLive() const {
    // Evita que las asignaciones internas del vector se auto-registren
    ScopedHookGuard guard;

    std::lock_guard<std::mutex> lock(mu_);
    std::vector<AllocationRecord> out;
    out.reserve(live_.size());
    for (const auto& kv : live_) out.push_back(kv.second);
    return out;
}

// === Metricas ===

// Devuelve los bytes actualmente en uso
std::size_t MemoryTracker::activeBytes() const {
    std::lock_guard<std::mutex> lock(mu_);
    return active_bytes_;
}

// Devuelve el maximo historico de bytes usados
std::size_t MemoryTracker::peakBytes() const {
    std::lock_guard<std::mutex> lock(mu_);
    return peak_bytes_;
}

// Devuelve el numero total de asignaciones realizadas
std::size_t MemoryTracker::totalAllocs() const {
    std::lock_guard<std::mutex> lock(mu_);
    return total_allocs_;
}

// Devuelve el numero de asignaciones actualmente activas
std::size_t MemoryTracker::activeAllocs() const {
    std::lock_guard<std::mutex> lock(mu_);
    return active_allocs_;
}

// Metodo auxiliar (actualmente no hace nada, reservado para pruebas)
void MemoryTracker::resetForTesting() {
    // Por ahora no-op. // TODO: permitir reset opcional bajo flag de desarrollo
}

} // namespace mp

