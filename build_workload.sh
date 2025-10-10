#!/bin/bash

# Script completo para compilar el workload con el profiler integrado
# Ejecutar desde el directorio MemoryProfiler-Workload

set -e

echo "=========================================="
echo "Memory Profiler Workload - Build Script"
echo "=========================================="
echo ""

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Función para imprimir mensajes
print_step() {
    echo -e "${GREEN}==>${NC} $1"
}

print_error() {
    echo -e "${RED}ERROR:${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}WARNING:${NC} $1"
}

# Verificar que estamos en el directorio correcto
if [ ! -f "CMakeLists.txt" ]; then
    print_error "CMakeLists.txt no encontrado. ¿Estás en el directorio MemoryProfiler-Workload?"
    exit 1
fi

# Paso 1: Verificar/Copiar archivos del profiler
print_step "Verificando archivos del profiler..."

if [ ! -d "profiler" ]; then
    print_warning "Directorio profiler no encontrado. Ejecutando setup..."
    
    if [ -f "setup_profiler.sh" ]; then
        bash setup_profiler.sh
    else
        print_error "setup_profiler.sh no encontrado. Por favor ejecuta primero el script de setup."
        exit 1
    fi
else
    echo "  ✓ Directorio profiler encontrado"
fi

# Verificar archivos críticos
CRITICAL_FILES=(
    "profiler/include/ProfilerAPI.hpp"
    "profiler/include/ProfilerNew.hpp"
    "profiler/include/SocketClient.hpp"
    "profiler/include/MemoryTracker.hpp"
    "profiler/src/OperatorOverrides.cpp"
)

MISSING=0
for file in "${CRITICAL_FILES[@]}"; do
    if [ ! -f "$file" ]; then
        print_error "Archivo crítico no encontrado: $file"
        MISSING=1
    fi
done

if [ $MISSING -eq 1 ]; then
    print_error "Faltan archivos del profiler. Ejecuta setup_profiler.sh primero."
    exit 1
fi

echo "  ✓ Todos los archivos del profiler verificados"
echo ""

# Paso 2: Limpiar build anterior
print_step "Limpiando build anterior..."
if [ -d "build" ]; then
    rm -rf build
    echo "  ✓ Directorio build eliminado"
fi

# Paso 3: Crear directorio de build
print_step "Creando directorio de build..."
mkdir -p build
cd build
echo "  ✓ Directorio build creado"
echo ""

# Paso 4: Configurar con CMake
print_step "Configurando proyecto con CMake..."
echo ""

cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DMP_USE_API=ON \
      -DMP_MAX_MEM_MB=300 \
      ..

if [ $? -ne 0 ]; then
    print_error "Configuración de CMake falló"
    exit 1
fi

echo ""
echo "  ✓ Proyecto configurado exitosamente"
echo ""

# Paso 5: Compilar
print_step "Compilando proyecto..."
echo ""

cmake --build . -j$(nproc)

if [ $? -ne 0 ]; then
    print_error "Compilación falló"
    exit 1
fi

echo ""
echo "  ✓ Compilación exitosa"
echo ""

# Paso 6: Verificar ejecutable
print_step "Verificando ejecutable..."

if [ ! -f "mp_workload" ]; then
    print_error "Ejecutable mp_workload no encontrado"
    exit 1
fi

echo "  ✓ Ejecutable mp_workload generado correctamente"
echo ""

# Resumen
echo "=========================================="
echo "Build completado exitosamente!"
echo "=========================================="
echo ""
echo "Ejecutable: $(pwd)/mp_workload"
echo ""
echo "Para ejecutar el workload:"
echo "  cd build"
echo "  ./mp_workload --threads 4 --seconds 10"
echo ""
echo "Opciones útiles:"
echo "  --threads N         : Número de hilos (default: 2)"
echo "  --seconds S         : Duración en segundos (default: 6)"
echo "  --scale K           : Factor de escala (default: 1.0)"
echo "  --leak-rate P       : Tasa de fugas 0.0-1.0 (default: 0.05)"
echo "  --burst-size B      : Tamaño de ráfagas (default: 500)"
echo "  --no-leaks          : Deshabilitar fugas de memoria"
echo "  --quiet             : Reducir salida de logs"
echo ""
echo "Antes de ejecutar, asegúrate de iniciar la GUI del profiler:"
echo "  cd ../Memory-Profiler/build"
echo "  ./memory_profiler_app"
echo ""