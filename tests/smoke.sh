#!/bin/bash

# Smoke test for mp_workload
# This script compiles and runs a basic test to verify functionality

set -e  # Exit on any error

echo "=== Memory Profiler Workload Smoke Test ==="

# Get the directory containing this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"

echo "Project root: $PROJECT_ROOT"
echo "Build directory: $BUILD_DIR"

# Clean and create build directory
echo "Setting up build directory..."
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo "Configuring with CMake..."
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DMP_USE_API=OFF -DMP_MAX_MEM_MB=100 "$PROJECT_ROOT"

# Build the project
echo "Building project..."
cmake --build . -j$(nproc)

# Verify executable exists
if [ ! -f "./mp_workload" ]; then
    echo "ERROR: mp_workload executable not found!"
    exit 1
fi

echo "Build successful!"

# Run basic functionality test
echo "Running basic functionality test..."
echo "Command: ./mp_workload --seconds 3 --threads 2 --quiet"

# Capture output
OUTPUT=$(./mp_workload --seconds 3 --threads 2 --quiet 2>&1)
EXIT_CODE=$?

echo "Exit code: $EXIT_CODE"
echo "Output:"
echo "$OUTPUT"

# Check exit code
if [ $EXIT_CODE -ne 0 ]; then
    echo "ERROR: mp_workload exited with code $EXIT_CODE"
    exit 1
fi

# Check for expected output patterns
echo "Verifying output..."

# Check for summary section
if ! echo "$OUTPUT" | grep -q "=== WORKLOAD SUMMARY ==="; then
    echo "ERROR: Missing workload summary section"
    exit 1
fi

# Check for memory statistics
if ! echo "$OUTPUT" | grep -q "Total allocations:"; then
    echo "ERROR: Missing allocation statistics"
    exit 1
fi

# Check for leak statistics (should show some leaks by default)
if ! echo "$OUTPUT" | grep -q "Leak Statistics:"; then
    echo "ERROR: Missing leak statistics"
    exit 1
fi

# Check for module breakdown
if ! echo "$OUTPUT" | grep -q "Module Breakdown:"; then
    echo "ERROR: Missing module breakdown"
    exit 1
fi

# Check for expected modules
EXPECTED_MODULES=("AllocStorm" "LeakFactory" "Fragmenter" "VectorChurn" "TreeFactory")
for module in "${EXPECTED_MODULES[@]}"; do
    if ! echo "$OUTPUT" | grep -q "$module:"; then
        echo "ERROR: Missing module $module in breakdown"
        exit 1
    fi
done

echo "All checks passed!"

# Test with no-leaks option
echo "Testing no-leaks option..."
NO_LEAKS_OUTPUT=$(./mp_workload --seconds 2 --threads 1 --no-leaks --quiet 2>&1)
NO_LEAKS_EXIT_CODE=$?

if [ $NO_LEAKS_EXIT_CODE -ne 0 ]; then
    echo "ERROR: no-leaks test failed with exit code $NO_LEAKS_EXIT_CODE"
    exit 1
fi

# Verify no leaks were created
if ! echo "$NO_LEAKS_OUTPUT" | grep -q "Total leaks: 0"; then
    echo "ERROR: Expected 0 leaks with --no-leaks option"
    exit 1
fi

echo "No-leaks test passed!"

# Test help option
echo "Testing help option..."
HELP_OUTPUT=$(./mp_workload --help 2>&1)
HELP_EXIT_CODE=$?

if [ $HELP_EXIT_CODE -ne 0 ]; then
    echo "ERROR: Help option failed with exit code $HELP_EXIT_CODE"
    exit 1
fi

if ! echo "$HELP_OUTPUT" | grep -q "Usage:"; then
    echo "ERROR: Help output missing usage information"
    exit 1
fi

echo "Help test passed!"

# Test error handling (invalid arguments)
echo "Testing error handling..."
ERROR_OUTPUT=$(./mp_workload --threads 0 2>&1)
ERROR_EXIT_CODE=$?

if [ $ERROR_EXIT_CODE -eq 0 ]; then
    echo "ERROR: Expected non-zero exit code for invalid arguments"
    exit 1
fi

if ! echo "$ERROR_OUTPUT" | grep -q "Error:"; then
    echo "ERROR: Expected error message for invalid arguments"
    exit 1
fi

echo "Error handling test passed!"

echo ""
echo "=== ALL TESTS PASSED ==="
echo "The memory profiler workload is working correctly!"
echo ""
echo "You can now:"
echo "1. Run the workload with your memory profiler"
echo "2. Test different configurations with various command-line options"
echo "3. Build with MP_USE_API=ON to test profiler API integration"
echo ""
echo "Example commands:"
echo "  ./mp_workload --threads 4 --seconds 10 --scale 2.0"
echo "  ./mp_workload --no-leaks --threads 2 --seconds 5"
echo "  ./mp_workload --leak-rate 0.1 --burst-size 1000"