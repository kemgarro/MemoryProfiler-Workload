#include "SocketClient.hpp"
#include "ProfilerAPI.hpp"    // mp::api::{getMetricsJson,getSnapshotJson}
#include "Callsite.hpp"       // bandera de anti reentrancy definida aqui

#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// POSIX
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <fcntl.h>

namespace mp {

// Esta variable global esta definida en otro archivo (OperatorOverrides.cpp)
// Se usa para evitar que el sistema se auto-intercepte mientras envia datos
extern thread_local bool in_hook;

// RAII guard: marca la variable in_hook como activa durante la vida del objeto
// Esto evita recursividad infinita al generar strings o JSON
struct AntiReentry {
    bool prev{};
    AntiReentry() : prev(mp::in_hook) { mp::in_hook = true; }
    ~AntiReentry() { mp::in_hook = prev; }
};

// --------------------------- helpers ---------------------------

// Intenta conectar con un servidor TCP en host:port con timeout
static int connectToServer(const std::string& host, uint16_t port, int timeout_ms) {
    struct addrinfo hints{};
    hints.ai_family   = AF_UNSPEC;   // IPv4 o IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Convertir puerto a string para getaddrinfo
    char port_str[16];
    std::snprintf(port_str, sizeof(port_str), "%u", static_cast<unsigned>(port));

    struct addrinfo* res = nullptr;
    int gai = ::getaddrinfo(host.c_str(), port_str, &hints, &res);
    if (gai != 0) {
        return -1;
    }

    int sock = -1;
    for (auto rp = res; rp != nullptr; rp = rp->ai_next) {
        int s = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (s < 0) continue;

        // Configurar socket como no bloqueante para poder aplicar timeout
        int flags = ::fcntl(s, F_GETFL, 0);
        if (flags >= 0) ::fcntl(s, F_SETFL, flags | O_NONBLOCK);

        int rc = ::connect(s, rp->ai_addr, rp->ai_addrlen);
        if (rc == 0) {
            sock = s; // conexion inmediata exitosa
            break;
        }
        if (errno == EINPROGRESS) {
            // Esperar hasta timeout para ver si conecta
            struct pollfd pfd{ s, POLLOUT, 0 };
            int prc = ::poll(&pfd, 1, timeout_ms);
            if (prc == 1 && (pfd.revents & POLLOUT)) {
                int err = 0; socklen_t len = sizeof(err);
                if (::getsockopt(s, SOL_SOCKET, SO_ERROR, &err, &len) == 0 && err == 0) {
                    sock = s; // conexion completada
                    break;
                }
            }
        }

        ::close(s); // fallo -> cerrar socket y probar siguiente direccion
    }

    ::freeaddrinfo(res);

    // Si se conecto, volver a modo bloqueante para IO mas sencillo
    if (sock >= 0) {
        int flags = ::fcntl(sock, F_GETFL, 0);
        if (flags >= 0) ::fcntl(sock, F_SETFL, flags & ~O_NONBLOCK);
    }
    return sock;
}

// Envia todos los datos en un bucle hasta completar o fallar
static bool sendAll(int fd, const char* data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = ::send(fd, data + sent, len - sent, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EINTR) continue; // interrupcion -> reintentar
            return false; // fallo
        }
        sent += static_cast<size_t>(n);
    }
    return true;
}

// Elimina espacios y saltos de linea al inicio y fin del string
static std::string trimCopy(const std::string& s) {
    size_t i = 0, j = s.size();
    while (i < j && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) ++i;
    while (j > i && (s[j-1] == ' ' || s[j-1] == '\t' || s[j-1] == '\r' || s[j-1] == '\n')) --j;
    return s.substr(i, j - i);
}

// --------------------------- SocketClient impl ---------------------------

class SocketClient::Impl {
public:
    Impl() = default;
    ~Impl() { stop(); }

    // Inicia el cliente en un hilo de fondo
    void start(const std::string& host, uint16_t port) {
        std::lock_guard<std::mutex> lk(m_);
        if (running_) return;
        host_ = host;
        port_ = port;
        running_ = true;
        worker_ = std::thread(&Impl::runLoop, this);
    }

    // Detiene el cliente y espera al hilo
    void stop() {
        {
            std::lock_guard<std::mutex> lk(m_);
            if (!running_) return;
            running_ = false;
        }
        if (worker_.joinable()) worker_.join();
        closeSocket();
    }

    bool isRunning() const noexcept { return running_; }

private:
    // Cierra el socket si esta abierto
    void closeSocket() {
        if (sock_ >= 0) {
            ::close(sock_);
            sock_ = -1;
        }
    }

    // Bucle principal del cliente
    void runLoop() {
        constexpr int   kConnectTimeoutMs = 2000;
        constexpr int   kPollTickMs       = 50;     // frecuencia de revision
        constexpr int   kMetricsMs        = 200;    // intervalo para enviar metricas
        constexpr size_t kReadBuf         = 4096;

        std::string rxBuffer;
        rxBuffer.reserve(8 * 1024);

        auto next_metrics = std::chrono::steady_clock::now();

        int backoff_ms = 200; // tiempo de espera entre reintentos de conexion
        while (true) {
            if (!running_) break;

            // Asegurar que hay conexion
            if (sock_ < 0) {
                int s = connectToServer(host_, port_, kConnectTimeoutMs);
                if (s < 0) {
                    // fallo -> esperar con backoff
                    std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
                    backoff_ms = std::min(backoff_ms * 2, 3000);
                    continue;
                }
                sock_ = s;
                backoff_ms = 200;
                next_metrics = std::chrono::steady_clock::now(); // enviar metricas pronto
            }

            // Calcular cuanto esperar antes de enviar metricas
            auto now = std::chrono::steady_clock::now();
            int timeout_ms = kPollTickMs;
            if (now < next_metrics) {
                auto remain = std::chrono::duration_cast<std::chrono::milliseconds>(next_metrics - now).count();
                timeout_ms = std::min(timeout_ms, static_cast<int>(remain));
            } else {
                timeout_ms = 0; // ya toca enviar
            }

            // Esperar lectura con poll
            struct pollfd pfd{ sock_, POLLIN, 0 };
            int prc = ::poll(&pfd, 1, timeout_ms);
            if (prc < 0) {
                closeSocket(); // error -> reconectar
                continue;
            }

            // Datos disponibles para leer
            if (prc > 0 && (pfd.revents & POLLIN)) {
                char buf[kReadBuf];
                ssize_t n = ::recv(sock_, buf, sizeof(buf), 0);
                if (n <= 0) {
                    closeSocket(); // cerrado por peer -> reconectar
                    continue;
                }
                rxBuffer.append(buf, static_cast<size_t>(n));

                // Procesar comandos terminados en salto de linea
                for (;;) {
                    auto pos = rxBuffer.find('\n');
                    if (pos == std::string::npos) break;
                    std::string line = trimCopy(rxBuffer.substr(0, pos));
                    rxBuffer.erase(0, pos + 1);

                    if (line == "SNAPSHOT") {
                        AntiReentry guard;
                        std::string json = mp::api::getSnapshotJson();
                        json.push_back('\n');
                        if (!sendAll(sock_, json.data(), json.size())) {
                            closeSocket();
                            break;
                        }
                    }
                    // Aqui se pueden agregar mas comandos en el futuro
                }
            }

            // Enviar metricas periodicas
            now = std::chrono::steady_clock::now();
            if (now >= next_metrics) {
                next_metrics = now + std::chrono::milliseconds(kMetricsMs);

                AntiReentry guard;
                std::string json = mp::api::getMetricsJson();
                json.push_back('\n');
                if (!sendAll(sock_, json.data(), json.size())) {
                    closeSocket();
                    continue;
                }
            }
        }

        closeSocket();
    }

private:
    mutable std::mutex m_;
    std::thread        worker_;
    std::atomic<bool>  running_{false};

    std::string host_{"127.0.0.1"};
    uint16_t    port_{7777};
    int         sock_{-1};
};

// --------------------------- SocketClient API ---------------------------

// API publica que envuelve la implementacion interna Impl
SocketClient::SocketClient() : impl_(new Impl()) {}
SocketClient::~SocketClient() { if (impl_) { impl_->stop(); delete impl_; } }

void SocketClient::start(const std::string& host, uint16_t port) { impl_->start(host, port); }
void SocketClient::stop()                                        { impl_->stop(); }
bool SocketClient::isRunning() const noexcept                    { return impl_->isRunning(); }

} // namespace mp
