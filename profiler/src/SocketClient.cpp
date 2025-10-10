#include "SocketClient.hpp"
#include "ProfilerAPI.hpp"
#include "Callsite.hpp"

#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <iostream>  // ← AGREGAR para debug

// POSIX
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <fcntl.h>

#include "../include/ProfilerNew.hpp"

namespace mp {

extern thread_local bool in_hook;

struct AntiReentry {
    bool prev{};
    AntiReentry() : prev(mp::in_hook) { mp::in_hook = true; }
    ~AntiReentry() { mp::in_hook = prev; }
};

// --------------------------- helpers ---------------------------

static int connectToServer(const std::string& host, uint16_t port, int timeout_ms) {
    struct addrinfo hints{};
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

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

        int flags = ::fcntl(s, F_GETFL, 0);
        if (flags >= 0) ::fcntl(s, F_SETFL, flags | O_NONBLOCK);

        int rc = ::connect(s, rp->ai_addr, rp->ai_addrlen);
        if (rc == 0) {
            sock = s;
            break;
        }
        if (errno == EINPROGRESS) {
            struct pollfd pfd{ s, POLLOUT, 0 };
            int prc = ::poll(&pfd, 1, timeout_ms);
            if (prc == 1 && (pfd.revents & POLLOUT)) {
                int err = 0; socklen_t len = sizeof(err);
                if (::getsockopt(s, SOL_SOCKET, SO_ERROR, &err, &len) == 0 && err == 0) {
                    sock = s;
                    break;
                }
            }
        }

        ::close(s);
    }

    ::freeaddrinfo(res);

    if (sock >= 0) {
        int flags = ::fcntl(sock, F_GETFL, 0);
        if (flags >= 0) ::fcntl(sock, F_SETFL, flags & ~O_NONBLOCK);
    }
    return sock;
}

static bool sendAll(int fd, const char* data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = ::send(fd, data + sent, len - sent, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        sent += static_cast<size_t>(n);
    }
    return true;
}

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

    void start(const std::string& host, uint16_t port) {
        std::lock_guard<std::mutex> lk(m_);
        if (running_) return;
        host_ = host;
        port_ = port;
        running_ = true;
        worker_ = std::thread(&Impl::runLoop, this);
    }

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
    void closeSocket() {
        if (sock_ >= 0) {
            ::close(sock_);
            sock_ = -1;
        }
    }

    void runLoop() {
        constexpr int   kConnectTimeoutMs = 2000;
        constexpr int   kPollTickMs       = 50;
        constexpr int   kMetricsMs        = 200;
        constexpr size_t kReadBuf         = 4096;

        std::string rxBuffer;
        rxBuffer.reserve(8 * 1024);

        auto next_metrics = std::chrono::steady_clock::now();

        int backoff_ms = 200;
        while (true) {
            if (!running_) break;

            // Asegurar conexión
            if (sock_ < 0) {
                std::cout << "[SocketClient] Intentando conectar a " << host_ << ":" << port_ << "...\n";
                int s = connectToServer(host_, port_, kConnectTimeoutMs);
                if (s < 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(backoff_ms));
                    backoff_ms = std::min(backoff_ms * 2, 3000);
                    continue;
                }
                sock_ = s;
                backoff_ms = 200;
                next_metrics = std::chrono::steady_clock::now();
                std::cout << "[SocketClient] Conectado exitosamente!\n";
            }

            auto now = std::chrono::steady_clock::now();
            int timeout_ms = kPollTickMs;
            if (now < next_metrics) {
                auto remain = std::chrono::duration_cast<std::chrono::milliseconds>(next_metrics - now).count();
                timeout_ms = std::min(timeout_ms, static_cast<int>(remain));
            } else {
                timeout_ms = 0;
            }

            // Poll para lectura
            struct pollfd pfd{ sock_, POLLIN, 0 };
            int prc = ::poll(&pfd, 1, timeout_ms);
            if (prc < 0) {
                std::cout << "[SocketClient] Error en poll, reconectando...\n";
                closeSocket();
                continue;
            }

            // Leer datos disponibles
            if (prc > 0 && (pfd.revents & POLLIN)) {
                char buf[kReadBuf];
                ssize_t n = ::recv(sock_, buf, sizeof(buf), 0);
                if (n <= 0) {
                    std::cout << "[SocketClient] Conexión cerrada por peer, reconectando...\n";
                    closeSocket();
                    continue;
                }
                rxBuffer.append(buf, static_cast<size_t>(n));

                // Procesar líneas completas
                for (;;) {
                    auto pos = rxBuffer.find('\n');
                    if (pos == std::string::npos) break;

                    std::string line = trimCopy(rxBuffer.substr(0, pos));
                    rxBuffer.erase(0, pos + 1);

                    std::cout << "[SocketClient] Comando recibido: '" << line << "'\n";

                    if (line == "SNAPSHOT") {
                        std::cout << "[SocketClient] Procesando comando SNAPSHOT...\n";
                        AntiReentry guard;
                        std::string json = mp::api::getSnapshotJson();
                        json.push_back('\n');

                        std::cout << "[SocketClient] Enviando snapshot (" << json.size() << " bytes)...\n";

                        if (!sendAll(sock_, json.data(), json.size())) {
                            std::cout << "[SocketClient] Error al enviar snapshot, reconectando...\n";
                            closeSocket();
                            break;
                        }

                        std::cout << "[SocketClient] Snapshot enviado exitosamente!\n";
                    }
                }
            }

            // Enviar métricas periódicas
            now = std::chrono::steady_clock::now();
            if (now >= next_metrics) {
                next_metrics = now + std::chrono::milliseconds(kMetricsMs);

                AntiReentry guard;
                std::string json = mp::api::getMetricsJson();
                json.push_back('\n');

                if (!sendAll(sock_, json.data(), json.size())) {
                    std::cout << "[SocketClient] Error al enviar métricas, reconectando...\n";
                    closeSocket();
                    continue;
                }
            }
        }

        closeSocket();
        std::cout << "[SocketClient] Hilo de trabajo terminado.\n";
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

SocketClient::SocketClient() : impl_(MP_NEW_FT(Impl)) {}
SocketClient::~SocketClient() { if (impl_) { impl_->stop(); delete impl_; } }

void SocketClient::start(const std::string& host, uint16_t port) { impl_->start(host, port); }
void SocketClient::stop()                                        { impl_->stop(); }
bool SocketClient::isRunning() const noexcept                    { return impl_->isRunning(); }

} // namespace mp