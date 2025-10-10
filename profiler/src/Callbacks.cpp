#include "../include/Callbacks.hpp"
#include <mutex>

namespace mp {

  // Variable global estatica que guarda los callbacks registrados
  static Callbacks g_cb;

  // Bandera usada para asegurar que la inicializacion solo ocurre una vez
  static std::once_flag g_init_once;

  // Funcion que inicializa todos los callbacks con funciones vacias (no hacen nada)
  static void init_noop() {
    g_cb.onAlloc    = [](void*, std::size_t, const char*, const char*, int, bool){}; // No hace nada al asignar memoria
    g_cb.onFree     = [](void*){};                                  // No hace nada al liberar memoria
    g_cb.bytesInUse = []{ return std::size_t(0); };                 // Siempre retorna 0
    g_cb.peakBytes  = []{ return std::size_t(0); };                 // Siempre retorna 0
    g_cb.allocCount = []{ return std::size_t(0); };                 // Siempre retorna 0
    g_cb.snapshot   = []{ return std::uint64_t(0); };               // Siempre retorna 0
    g_cb.liveBlocks = []{ return std::vector<BlockInfo>{}; };       // Siempre retorna un vector vacio
  }

  // Funcion para registrar nuevos callbacks desde afuera
  // Si algun callback no es proporcionado, se reemplaza por uno vacio
  void register_callbacks(const Callbacks& c) {
    // Se asegura que init_noop solo se ejecute una vez en todo el programa
    std::call_once(g_init_once, init_noop);

    // Guardamos los callbacks proporcionados por el usuario
    g_cb = c;

    // Si alguno de los callbacks no fue asignado, se reemplaza con version vacia
    if (!g_cb.onAlloc)    g_cb.onAlloc = [](void*, std::size_t, const char*, const char*, int, bool){};
    if (!g_cb.onFree)     g_cb.onFree     = [](void*){};
    if (!g_cb.bytesInUse) g_cb.bytesInUse = []{ return std::size_t(0); };
    if (!g_cb.peakBytes)  g_cb.peakBytes  = []{ return std::size_t(0); };
    if (!g_cb.allocCount) g_cb.allocCount = []{ return std::size_t(0); };
    if (!g_cb.snapshot)   g_cb.snapshot   = []{ return std::uint64_t(0); };
    if (!g_cb.liveBlocks) g_cb.liveBlocks = []{ return std::vector<BlockInfo>{}; };
  }

  // Funcion para obtener los callbacks actuales
  // Siempre asegura que al menos existan callbacks vacios
  const Callbacks& get_callbacks() {
    std::call_once(g_init_once, init_noop);
    return g_cb;
  }

} // namespace mp