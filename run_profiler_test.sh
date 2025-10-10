#!/bin/bash

# Script para ejecutar una prueba completa del Memory Profiler con el Workload
# Este script:
# 1. Inicia la GUI del profiler en background
# 2. Espera a que esté lista
# 3. Ejecuta el workload
# 4. Mantiene la GUI abierta para análisis

set -e

echo "=========================================="
echo "Memory Profiler - Test Completo"
echo "=========================================="
echo ""

# Colores
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

print_step() {
    echo -e "${GREEN}==>${NC} $1"
}

print_error() {
    echo -e "${RED}ERROR:${NC} $1"
}

print_info() {
    echo -e "${YELLOW}INFO:${NC} $1"
}

# Directorios
PROFILER_DIR="../Memory-Profiler"
WORKLOAD_DIR="."

# Verificar que los ejecutables existen
print_step "Verificando ejecutables..."

if [ ! -f "$PROFILER_DIR/build/memory_profiler_app" ]; then
    print_error "GUI del profiler no encontrada en $PROFILER_DIR/build/"
    echo "Por favor compila primero el Memory Profiler:"
    echo "  cd $PROFILER_DIR"
    echo "  mkdir -p build && cd build"
    echo "  cmake .."
    echo "  cmake --build . -j"
    exit 1
fi

if [ ! -f "$WORKLOAD_DIR/build/mp_workload" ]; then
    print_error "Workload no encontrado en $WORKLOAD_DIR/build/"
    echo "Por favor compila primero el workload con:"
    echo "  bash build_workload.sh"
    exit 1
fi

echo "  ✓ Ejecutables verificados"
echo ""

# Configuración de la prueba
THREADS=${1:-4}
SECONDS=${2:-15}
SCALE=${3:-1.5}
LEAK_RATE=${4:-0.08}

print_info "Configuración de la prueba:"
echo "  Threads: $THREADS"
echo "  Duration: ${SECONDS}s"
echo "  Scale: $SCALE"
echo "  Leak rate: $LEAK_RATE"
echo ""

# Función para limpiar procesos al salir
cleanup() {
    if [ ! -z "$GUI_PID" ]; then
        print_step "Deteniendo GUI del profiler..."
        kill $GUI_PID 2>/dev/null || true
        wait $GUI_PID 2>/dev/null || true
    fi
}

trap cleanup EXIT INT TERM

# Paso 1: Iniciar la GUI
print_step "Iniciando GUI del Memory Profiler..."
cd "$PROFILER_DIR/build"
./memory_profiler_app &
GUI_PID=$!

echo "  ✓ GUI iniciada (PID: $GUI_PID)"
echo ""

# Esperar a que la GUI esté lista (servidor escuchando en puerto 7777)
print_step "Esperando a que el servidor esté listo..."
MAX_WAIT=10
WAITED=0

while [ $WAITED -lt $MAX_WAIT ]; do
    if netstat -tuln 2>/dev/null | grep -q ":7777 " || \
       ss -tuln 2>/dev/null | grep -q ":7777 "; then
        echo "  ✓ Servidor listo en puerto 7777"
        break
    fi
    
    sleep 1
    WAITED=$((WAITED + 1))
    echo "  Esperando... ($WAITED/$MAX_WAIT)"
done

if [ $WAITED -eq $MAX_WAIT ]; then
    print_error "Timeout esperando al servidor del profiler"
    exit 1
fi

sleep 2  # Espera adicional para asegurar estabilidad
echo ""

# Paso 2: Ejecutar el workload
print_step "Ejecutando workload con profiler..."
cd "$WORKLOAD_DIR/build"

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║  WORKLOAD EJECUTÁNDOSE - OBSERVA LA GUI DEL PROFILER      ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

./mp_workload \
    --threads $THREADS \
    --seconds $SECONDS \
    --scale $SCALE \
    --leak-rate $LEAK_RATE \
    --burst-size 800

WORKLOAD_EXIT=$?

echo ""
echo "=========================================="
if [ $WORKLOAD_EXIT -eq 0 ]; then
    echo -e "${GREEN}✓ Workload completado exitosamente${NC}"
else
    echo -e "${RED}✗ Workload falló con código $WORKLOAD_EXIT${NC}"
fi
echo "=========================================="
echo ""

# Paso 3: Mantener la GUI abierta
print_info "La GUI del profiler permanecerá abierta para análisis."
print_info "Puedes:"
echo "  - Revisar las métricas en la pestaña 'Vista general'"
echo "  - Hacer clic en 'Snapshot' para capturar el estado final"
echo "  - Revisar el 'Mapa de memoria' para ver los bloques"
echo "  - Analizar 'Memory leaks' para ver las fugas detectadas"
echo "  - Ver 'Asignación por archivo' para estadísticas por fuente"
echo ""
print_info "Presiona Ctrl+C para cerrar la GUI y finalizar"
echo ""

# Esperar a que el usuario cierre la GUI
wait $GUI_PID

echo ""
print_step "Test completado. ¡Revisa los resultados en la GUI!"