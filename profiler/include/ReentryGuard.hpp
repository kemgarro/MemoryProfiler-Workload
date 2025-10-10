#pragma once

// Declaracion de la variable thread_local usada para controlar reentrancia
namespace mp {
    extern thread_local bool in_hook;
}

namespace mp {

    // RAII guard para manejar la variable in_hook
    // Activa in_hook durante la vida del objeto y la restaura al salir del scope
    struct ScopedHookGuard {
        bool prev{}; // guarda el valor previo de in_hook
        ScopedHookGuard() : prev(mp::in_hook) {
            mp::in_hook = true; // activa flag para evitar recursion
        }
        ~ScopedHookGuard() {
            mp::in_hook = prev; // restaura valor previo al salir del scope
        }
    };

} // namespace mp