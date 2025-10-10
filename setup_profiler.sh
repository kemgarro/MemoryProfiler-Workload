#!/bin/bash

# Script para configurar el profiler en el workload
# Ejecutar desde la raíz del proyecto MemoryProfiler-Workload

set -e

echo "=== Configurando Memory Profiler para Workload ==="

# Directorios
PROFILER_SRC="../Proyecto_1/src/Library"
PROFILER_DEST="./profiler"

# Limpiar directorio destino si existe
if [ -d "$PROFILER_DEST" ]; then
    echo "Limpiando directorio $PROFILER_DEST..."
    rm -rf "$PROFILER_DEST"
fi

# Crear estructura de directorios
echo "Creando estructura de directorios..."
mkdir -p "$PROFILER_DEST/src"
mkdir -p "$PROFILER_DEST/include"

# Copiar archivos fuente
echo "Copiando archivos fuente..."
cp "$PROFILER_SRC/core/BlockInfo.cpp" "$PROFILER_DEST/src/"
cp "$PROFILER_SRC/core/Callbacks.cpp" "$PROFILER_DEST/src/"
cp "$PROFILER_SRC/core/CallbacksRegistration.cpp" "$PROFILER_DEST/src/"
cp "$PROFILER_SRC/core/MemoryTracker.cpp" "$PROFILER_DEST/src/"
cp "$PROFILER_SRC/core/OperatorOverrides.cpp" "$PROFILER_DEST/src/"
cp "$PROFILER_SRC/core/ProfilerAPI.cpp" "$PROFILER_DEST/src/"
cp "$PROFILER_SRC/core/Serializer.cpp" "$PROFILER_DEST/src/"
cp "$PROFILER_SRC/core/SocketClient.cpp" "$PROFILER_DEST/src/"

# Copiar archivos de encabezado
echo "Copiando archivos de encabezado..."
cp "$PROFILER_SRC/include/BlockInfo.hpp" "$PROFILER_DEST/include/"
cp "$PROFILER_SRC/include/Callbacks.hpp" "$PROFILER_DEST/include/"
cp "$PROFILER_SRC/include/CallbacksRegistration.hpp" "$PROFILER_DEST/include/"
cp "$PROFILER_SRC/include/Callsite.hpp" "$PROFILER_DEST/include/"
cp "$PROFILER_SRC/include/MemoryTracker.hpp" "$PROFILER_DEST/include/"
cp "$PROFILER_SRC/include/OperatorOverrides.hpp" "$PROFILER_DEST/include/"
cp "$PROFILER_SRC/include/ProfilerAPI.hpp" "$PROFILER_DEST/include/"
cp "$PROFILER_SRC/include/ProfilerNew.hpp" "$PROFILER_DEST/include/"
cp "$PROFILER_SRC/include/ReentryGuard.hpp" "$PROFILER_DEST/include/"
cp "$PROFILER_SRC/include/Serializer.hpp" "$PROFILER_DEST/include/"
cp "$PROFILER_SRC/include/SocketClient.hpp" "$PROFILER_DEST/include/"

echo "✅ Archivos copiados exitosamente"

# Verificar que todos los archivos existen
echo "Verificando archivos..."
MISSING=0

for file in BlockInfo.cpp Callbacks.cpp CallbacksRegistration.cpp MemoryTracker.cpp \
            OperatorOverrides.cpp ProfilerAPI.cpp Serializer.cpp SocketClient.cpp; do
    if [ ! -f "$PROFILER_DEST/src/$file" ]; then
        echo "❌ Falta: $PROFILER_DEST/src/$file"
        MISSING=1
    fi
done

for file in BlockInfo.hpp Callbacks.hpp CallbacksRegistration.hpp Callsite.hpp \
            MemoryTracker.hpp OperatorOverrides.hpp ProfilerAPI.hpp ProfilerNew.hpp \
            ReentryGuard.hpp Serializer.hpp SocketClient.hpp; do
    if [ ! -f "$PROFILER_DEST/include/$file" ]; then
        echo "❌ Falta: $PROFILER_DEST/include/$file"
        MISSING=1
    fi
done

if [ $MISSING -eq 0 ]; then
    echo "✅ Todos los archivos verificados"
else
    echo "❌ Faltan algunos archivos"
    exit 1
fi

echo ""
echo "=== Configuración completada ==="
echo "Ahora puedes compilar el workload con:"
echo "  mkdir -p build && cd build"
echo "  cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DMP_USE_API=ON .."
echo "  cmake --build . -j"