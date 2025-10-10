#pragma once
#include <cstdint>
#include <string>

namespace mp {

    /**
     * @brief Cliente TCP que envia periodicamente metricas en JSON
     *        y responde a solicitudes "SNAPSHOT" con snapshot JSON.
     *
     * Protocolo:
     *   - Salida: frames JSON separados por salto de linea (metrics, snapshot)
     *   - Entrada: lineas de texto; si la linea == "SNAPSHOT", se envia snapshot JSON
     *
     * Hilos:
     *   - start() crea un hilo en segundo plano; stop() lo une al hilo principal
     */
    class SocketClient {
    public:
        SocketClient();
        ~SocketClient();

        // Prohibir copia y asignacion
        SocketClient(const SocketClient&) = delete;
        SocketClient& operator=(const SocketClient&) = delete;

        /**
         * @brief Inicia el hilo del cliente. Si ya esta corriendo, no hace nada.
         * @param host Host del servidor (por ejemplo "127.0.0.1")
         * @param port Puerto del servidor (por ejemplo 7777)
         */
        void start(const std::string& host = "127.0.0.1", uint16_t port = 7777);

        /**
         * @brief Detiene el hilo del cliente si esta corriendo. Seguro llamar varias veces.
         */
        void stop();

        /**
         * @brief Retorna true si el hilo trabajador esta corriendo
         */
        bool isRunning() const noexcept;

    private:
        class Impl; // implementacion interna (pimpl idiom)
        Impl* impl_; // puntero a la implementacion real
    };

} // namespace mp