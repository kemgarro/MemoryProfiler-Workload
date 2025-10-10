#pragma once
#include <cstddef>

namespace mp {

    // Información de *dónde* se hizo la llamada a new (y qué tipo se intentó crear).
    struct CallsiteInfo {
        const char* file = nullptr;
        int         line = 0;
        const char* type_name = nullptr; // tip: puede venir de typeid(T).name()
    };

    // TLS: cada hilo mantiene su callsite actual.
    inline thread_local CallsiteInfo g_callsite;

    // Setters simples (pedidos en el enunciado)
    inline void setCallsite(const char* file, int line) noexcept {
        g_callsite.file = file;
        g_callsite.line = line;
    }
    inline void setTypeName(const char* tn) noexcept {
        g_callsite.type_name = tn;
    }

    // Get/clear utilitarios
    inline CallsiteInfo currentCallsite() noexcept { return g_callsite; }
    inline void clearCallsite() noexcept { g_callsite = {}; }

    // RAII para setear (y restaurar) el callsite durante la vida de un scope.
    class ScopedCallsite {
    public:
        ScopedCallsite(const char* file, int line, const char* type = nullptr) noexcept {
            prev_ = g_callsite;
            g_callsite.file = file;
            g_callsite.line = line;
            g_callsite.type_name = type;
        }
        ~ScopedCallsite() { g_callsite = prev_; }
    private:
        CallsiteInfo prev_{};
    };

} // namespace mp