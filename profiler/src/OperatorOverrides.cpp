#include "../include/OperatorOverrides.hpp"
#include "../include/ProfilerNew.hpp"
#include "../include/Callbacks.hpp"
#include "../include/Callsite.hpp"

#include <new>
#include <cstdlib>

// Variable thread_local que previene recursión infinita
// Si in_hook=true, significa que YA estamos dentro de un hook
// y NO debemos volver a llamar callbacks
namespace mp { thread_local bool in_hook = false; }

// === Sobrecarga del operador new ===
void* operator new(std::size_t sz) {
  // 1. Si tamaño es 0, ajustar a 1 (estándar C++)
  if (sz == 0) sz = 1;

  // 2. Asignar memoria con malloc (NO con new, ¡evita recursión!)
  void* p = std::malloc(sz);
  if (!p) throw std::bad_alloc{}; // Si falla, lanzar excepción

  // 3. Si NO estamos en recursión, registrar la asignación
  if (!mp::in_hook) {
    mp::in_hook = true; // Activar flag de recursión

    const auto& cb = mp::get_callbacks(); // Obtener callbacks registrados

    // CRÍTICO: Capturar callsite ANTES de cualquier operación
    // Esto obtiene: archivo, línea, tipo
    auto cs = mp::currentCallsite();

    // Notificar al sistema que se asignó memoria
    // Parámetros: ptr, tamaño, tipo, archivo, línea, es_array
    cb.onAlloc(p, sz, cs.type_name, cs.file, cs.line, false);

    // Limpiar callsite para la próxima asignación
    mp::clearCallsite();

    mp::in_hook = false; // Desactivar flag
  }
  return p; // Retornar el puntero asignado
}

// === Sobrecarga del operador delete ===
void operator delete(void* p) noexcept {
  if (!p) return;
  if (!mp::in_hook) {
    mp::in_hook = true;
    const auto& cb = mp::get_callbacks();
    cb.onFree(p);
    mp::in_hook = false;
  }
  std::free(p);
}

// === Sobrecarga del operador new[] ===
void* operator new[](std::size_t sz) {
  if (sz == 0) sz = 1;
  void* p = std::malloc(sz);
  if (!p) throw std::bad_alloc{};

  if (!mp::in_hook) {
    mp::in_hook = true;
    const auto& cb = mp::get_callbacks();

    // IMPORTANTE: Capturar callsite ANTES de cualquier operación
    auto cs = mp::currentCallsite();

    // Notificar asignación de arreglo
    cb.onAlloc(p, sz, cs.type_name, cs.file, cs.line, true);

    // Limpiar callsite DESPUÉS de notificar
    mp::clearCallsite();

    mp::in_hook = false;
  }
  return p;
}

// === Sobrecarga del operador delete[] ===
void operator delete[](void* p) noexcept {
  if (!p) return;
  if (!mp::in_hook) {
    mp::in_hook = true;
    const auto& cb = mp::get_callbacks();
    cb.onFree(p);
    mp::in_hook = false;
  }
  std::free(p);
}

// === Sobrecargas de delete con tamaño ===
void operator delete(void* p, std::size_t) noexcept { operator delete(p); }
void operator delete[](void* p, std::size_t) noexcept { operator delete[](p); }