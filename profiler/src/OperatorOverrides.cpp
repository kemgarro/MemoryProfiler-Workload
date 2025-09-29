#include "OperatorOverrides.hpp"
#include "ProfilerNew.hpp"
#include "Callbacks.hpp"
#include "Callsite.hpp"

#include <new>
#include <cstdlib>

// Flag de reentrancia visible desde otros TU (Translation Units)
// Se usa para evitar recursion infinita cuando dentro de un hook
// se ejecuta otra llamada a new/delete
namespace mp { thread_local bool in_hook = false; }

// === Sobrecarga del operador new ===
void* operator new(std::size_t sz) {
  if (sz == 0) sz = 1;                 // Nunca pedir 0 bytes, minimo 1
  void* p = std::malloc(sz);           // Reservamos memoria con malloc
  if (!p) throw std::bad_alloc{};      // Si malloc falla, lanzamos excepcion

  // Si no estamos dentro de un hook, registramos la asignacion
  if (!mp::in_hook) {
    mp::in_hook = true;                // Activamos flag de proteccion
    const auto& cb = mp::get_callbacks(); // Obtenemos callbacks registrados
    cb.onAlloc(p, sz, nullptr);        // Notificamos asignacion
    mp::in_hook = false;               // Desactivamos flag
  }
  return p;
}

// === Sobrecarga del operador delete ===
void operator delete(void* p) noexcept {
  if (!p) return;                      // Ignorar puntero nulo
  if (!mp::in_hook) {
    mp::in_hook = true;
    const auto& cb = mp::get_callbacks();
    cb.onFree(p);                      // Notificamos liberacion
    mp::in_hook = false;
  }
  std::free(p);                        // Liberamos memoria real
}

// === Sobrecarga del operador new[] ===
void* operator new[](std::size_t sz) {
  if (sz == 0) sz = 1;
  void* p = std::malloc(sz);
  if (!p) throw std::bad_alloc{};

  if (!mp::in_hook) {
    mp::in_hook = true;
    const auto& cb = mp::get_callbacks();
    cb.onAlloc(p, sz, nullptr);        // Notificamos asignacion de arreglo
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
    cb.onFree(p);                      // Notificamos liberacion de arreglo
    mp::in_hook = false;
  }
  std::free(p);
}

// === Sobrecargas de delete con tamaño ===
// Se implementan solo para evitar warnings del compilador
void operator delete(void* p, std::size_t) noexcept { operator delete(p); }
void operator delete[](void* p, std::size_t) noexcept { operator delete[](p); }
