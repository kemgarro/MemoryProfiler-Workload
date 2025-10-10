cmake_minimum_required(VERSION 3.16)
project(mp_workload VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export compile commands for tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type defaults to RelWithDebInfo for debugging
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# Compiler flags - no optimization to favor profiler
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -Wall -Wextra -Wpedantic")

# Options
option(MP_USE_API "Enable profiler API calls" ON)
set(MP_MAX_MEM_MB 300 CACHE STRING "Maximum memory usage in MB")

# Add definitions
add_definitions(-DMP_MAX_MEM_MB=${MP_MAX_MEM_MB})
if(MP_USE_API)
    add_definitions(-DMP_USE_API=1)
endif()

# Include directories for workload
include_directories(include)

# --------------------------------------------------
# Memory Profiler Integration
# --------------------------------------------------

# Check if profiler directory exists
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/profiler/include")
    message(FATAL_ERROR 
        "Profiler files not found in ${CMAKE_CURRENT_SOURCE_DIR}/profiler/\n"
        "Please run setup_profiler.sh first to copy the profiler files.")
endif()

# Profiler source files
set(PROFILER_SOURCES
    profiler/src/main.cpp
    profiler/src/BlockInfo.cpp
    profiler/src/Callbacks.cpp
    profiler/src/CallbacksRegistration.cpp
    profiler/src/MemoryTracker.cpp
    profiler/src/OperatorOverrides.cpp
    profiler/src/ProfilerAPI.cpp
    profiler/src/Serializer.cpp
    profiler/src/SocketClient.cpp
)

# Add profiler library
add_library(memory_profiler STATIC ${PROFILER_SOURCES})

# Include profiler headers
target_include_directories(memory_profiler PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/profiler/include
)

# Set profiler C++ standard
target_compile_features(memory_profiler PUBLIC cxx_std_17)

# Position independent code
set_target_properties(memory_profiler PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# Link with pthreads (required by SocketClient)
find_package(Threads REQUIRED)
target_link_libraries(memory_profiler PUBLIC Threads::Threads)

# --------------------------------------------------
# Workload Executable
# --------------------------------------------------

# Source files
set(WORKLOAD_SOURCES
    src/WorkloadConfig.cpp
    src/Types.cpp
    src/AllocStorm.cpp
    src/LeakFactory.cpp
    src/Fragmenter.cpp
    src/VectorChurn.cpp
    src/TreeFactory.cpp
    src/Utilities.cpp
    src/Node.cpp
)

# Create executable
add_executable(mp_workload ${WORKLOAD_SOURCES})

# Link profiler library FIRST (important for operator overrides)
target_link_libraries(mp_workload PRIVATE memory_profiler Threads::Threads)

# Include profiler headers in workload
target_include_directories(mp_workload PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/profiler/include
)

# Set output directory
set_target_properties(mp_workload PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Ensure profiler is built first
add_dependencies(mp_workload memory_profiler)

# --------------------------------------------------
# Installation (optional)
# --------------------------------------------------

install(TARGETS mp_workload
    RUNTIME DESTINATION bin
)

# Print configuration
message(STATUS "========================================")
message(STATUS "Memory Profiler Workload Configuration")
message(STATUS "========================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "MP_USE_API: ${MP_USE_API}")
message(STATUS "MP_MAX_MEM_MB: ${MP_MAX_MEM_MB}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Profiler library: memory_profiler")
message(STATUS "========================================")