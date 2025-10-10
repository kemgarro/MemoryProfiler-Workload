#pragma once
#include <string>
#include <cstdint>

namespace mp {

    void start();
    void stop();
    bool is_enabled();

    using SnapshotId = std::uint64_t;
    SnapshotId snapshot();

    // Reportes "puros"
    std::string summary_json();       // JSON: bytes_in_use, peak, alloc_count
    std::string live_allocs_csv();    // CSV: para tests o exportar

    // Mensajes para GUI (todo JSON)
    std::string summary_message_json();      // {"type":"SUMMARY","payload":{...}}
    std::string live_allocs_message_json();  // {"type":"LIVE_ALLOCS","payload":{"blocks":[...]}}

    struct ScopedSection {
        explicit ScopedSection(const char* name);
        ~ScopedSection();
    };

    // ---- Compatibilidad con demo / SocketClient ----
    namespace api {
        // Devuelven el "message JSON" listo para enviar por socket
        std::string getMetricsJson();   // wrapper -> summary_message_json()
        std::string getSnapshotJson();  // wrapper -> live_allocs_message_json()
    }

} // namespace mp