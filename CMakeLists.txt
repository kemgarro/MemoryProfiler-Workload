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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -Wall -Wextra -Wpedantic -Werror")

# Options
option(MP_USE_API "Enable profiler API calls" OFF)
set(MP_MAX_MEM_MB 300 CACHE STRING "Maximum memory usage in MB")

# Add definitions
add_definitions(-DMP_MAX_MEM_MB=${MP_MAX_MEM_MB})
if(MP_USE_API)
    add_definitions(-DMP_USE_API=1)
endif()

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/main.cpp
    src/WorkloadConfig.cpp
    src/Types.cpp
    src/AllocStorm.cpp
    src/LeakFactory.cpp
    src/Fragmenter.cpp
    src/VectorChurn.cpp
    src/TreeFactory.cpp
    src/Utilities.cpp
)

# Create executable
add_executable(mp_workload ${SOURCES})

# Set output directory
set_target_properties(mp_workload PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Print configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "MP_USE_API: ${MP_USE_API}")
message(STATUS "MP_MAX_MEM_MB: ${MP_MAX_MEM_MB}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")