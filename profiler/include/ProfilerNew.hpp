// src/Library/include/ProfilerNew.hpp
#pragma once
#include "Callsite.hpp"
#include <typeinfo>

// Macro para crear un objeto con registro del callsite y tipo
// Uso: MP_NEW_FT(MiClase, args...)
// Esto registra el archivo, linea y nombre de tipo, luego hace new
#define MP_NEW_FT(T, ...) ( mp::ScopedCallsite(__FILE__, __LINE__, typeid(T).name()), new T(__VA_ARGS__) )

// Macros para fijar manualmente el callsite y el nombre de tipo
#define MP_SET_CALLSITE()   ::mp::setCallsite(__FILE__, __LINE__)
#define MP_SET_TYPENAME(T)  ::mp::setTypeName(typeid(T).name())

// Macro para crear un arreglo con registro del callsite y tipo
// Uso: MP_NEW_ARRAY_FT(T, count)
// Esto registra el archivo, línea y nombre de tipo, luego hace new[]
#define MP_NEW_ARRAY_FT(T, count) ( mp::ScopedCallsite(__FILE__, __LINE__, typeid(T).name()), new T[count] )
