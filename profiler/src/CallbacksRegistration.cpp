#include "Callbacks.hpp"
#include "MemoryTracker.hpp"
#include "BlockInfo.hpp"
#include "Callsite.hpp"
#include "ReentryGuard.hpp"
#include <atomic>
#include <vector>
#include <string>

namespace mp {

// Contador global atomico para asignaciones de memoria
static std::atomic<std::uint64_t> g_alloc_id{0};

// Contador global atomico para snapshots (capturas de estado)
static std::atomic<std::uint64_t> g_snapshot_id{0};

// Esta funcion instala callbacks que usan el sistema MemoryTracker
// De esta forma, cada vez que se asigna o libera memoria, se registran los datos
void install_callbacks_with_memorytracker() {
    mp::Callbacks cb{}; // Se crea un objeto Callbacks vacio

    // Callback que se llama cada vez que se asigna memoria
    cb.onAlloc = [](void* p, std::size_t sz, const char* /*cs*/) {
        // Obtenemos la informacion del lugar donde se hizo la llamada (archivo, linea, tipo)
        auto cs = mp::currentCallsite();
        const bool is_array = false;

        // Registramos la asignacion en el MemoryTracker
        mp::MemoryTracker::instance().onAlloc(
            p, sz, cs.type_name, cs.file, cs.line, is_array
        );

        // Aumentamos el contador global de asignaciones
        (void)g_alloc_id.fetch_add(1, std::memory_order_relaxed);

        // Limpiamos el callsite despues de usarlo
        mp::clearCallsite();
    };

    // Callback que se llama cada vez que se libera memoria
    cb.onFree = [](void* p) {
        const bool is_array = false;
        mp::MemoryTracker::instance().onFree(p, is_array);
    };

    // Callback que devuelve la cantidad de memoria actualmente en uso
    cb.bytesInUse = [] { return mp::MemoryTracker::instance().activeBytes(); };

    // Callback que devuelve el maximo historico de memoria usada
    cb.peakBytes  = [] { return mp::MemoryTracker::instance().peakBytes();  };

    // Callback que devuelve el numero total de asignaciones
    cb.allocCount = [] { return mp::MemoryTracker::instance().totalAllocs();};

    // Callback que devuelve un id unico para cada snapshot
    cb.snapshot   = [] { return g_snapshot_id.fetch_add(1, std::memory_order_relaxed); };

    // Callback que devuelve una lista de bloques de memoria vivos (no liberados)
    cb.liveBlocks = [] {
        // Guard para evitar reentradas (protege contra recursion de hooks)
        mp::ScopedHookGuard guard;

        std::vector<BlockInfo> out; // Vector de resultados
        auto recs = mp::MemoryTracker::instance().snapshotLive(); // Obtenemos los bloques vivos
        out.reserve(recs.size());

        // Convertimos cada registro del tracker en un BlockInfo
        for (const auto& r : recs) {
            BlockInfo b{};
            b.ptr       = r.ptr;                              // Direccion de memoria
            b.size      = r.size;                             // Tamaño en bytes
            b.alloc_id  = g_alloc_id.fetch_add(1, std::memory_order_relaxed); // ID unico
            b.thread_id = r.thread_id;                        // Hilo que hizo la asignacion
            b.t_ns      = r.timestamp_ns;                     // Tiempo en nanosegundos

            // Si tenemos informacion de archivo y linea, la guardamos
            if (r.file && *r.file) {
                b.callsite = std::string(r.file) + ":" + std::to_string(r.line);
            } else {
                b.callsite = "?:0"; // Si no hay informacion, se pone un valor por defecto
            }

            // Agregamos el bloque a la lista
            out.push_back(std::move(b));
        }
        return out;
    };

    // Finalmente registramos todos los callbacks en el sistema
    mp::register_callbacks(cb);
}

} // namespace mp
