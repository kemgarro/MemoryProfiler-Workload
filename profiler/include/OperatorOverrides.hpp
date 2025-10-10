#pragma once
#include <cstddef>

// Declaracion de las sobrecargas globales de new/delete
void* operator new(std::size_t sz);
void  operator delete(void* p) noexcept;
void* operator new[](std::size_t sz);
void  operator delete[](void* p) noexcept;