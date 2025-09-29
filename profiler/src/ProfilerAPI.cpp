#include "ProfilerAPI.hpp"
#include "Callbacks.hpp"
#include "Serializer.hpp"
#include <atomic>

// Flag global atomico que indica si el profiler esta habilitado
namespace { std::atomic<bool> g_enabled{true}; }

namespace mp {

  // === Control del profiler ===

  // Inicia el profiler
  void start() { g_enabled.store(true, std::memory_order_relaxed); }

  // Detiene el profiler
  void stop() { g_enabled.store(false, std::memory_order_relaxed); }

  // Indica si el profiler esta habilitado
  bool is_enabled() { return g_enabled.load(std::memory_order_relaxed); }

  // === Snapshots y metricas ===

  // Obtiene un nuevo id de snapshot
  SnapshotId snapshot() {
    const auto& cb = get_callbacks();
    return cb.snapshot();
  }

  // Devuelve un resumen en formato JSON con metricas basicas
  std::string summary_json() {
    const auto& cb = get_callbacks();
    return make_summary_json(cb.bytesInUse(), cb.peakBytes(), cb.allocCount());
  }

  // Devuelve una lista de asignaciones vivas en formato CSV
  std::string live_allocs_csv() {
    const auto& cb = get_callbacks();
    return make_live_allocs_csv(cb.liveBlocks());
  }

  // Devuelve un mensaje JSON con el resumen de metricas
  std::string summary_message_json() {
    const auto& cb = get_callbacks();
    auto payload = make_summary_json(cb.bytesInUse(), cb.peakBytes(), cb.allocCount());
    return make_message_json("SUMMARY", payload);
  }

  // Devuelve un mensaje JSON con la lista de asignaciones vivas
  std::string live_allocs_message_json() {
    const auto& cb = get_callbacks();
    auto payload = make_live_allocs_json(cb.liveBlocks());
    return make_message_json("LIVE_ALLOCS", payload);
  }

  // === Secciones de medicion (scope) ===
  // Por ahora son no-op (no hacen nada)
  ScopedSection::ScopedSection(const char* /*name*/) {}
  ScopedSection::~ScopedSection() {}

  // === Wrappers de compatibilidad (ej. para SocketClient demo) ===
  namespace api {
    std::string getMetricsJson()  { return summary_message_json(); }
    std::string getSnapshotJson() { return live_allocs_message_json(); }
  }

} // namespace mp
