cmake_minimum_required(VERSION 3.20)
project(MonteCarloEngine VERSION 1.0 LANGUAGES CXX)

# C++20 Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
    
    # Optimization flags for Release build
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -march=native -mavx2 -mfma)
    endif()
elseif(MSVC)
    add_compile_options(/W4)
    
    # Optimization flags for Release build
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(/O2 /arch:AVX2)
    endif()
endif()

# Include directories
include_directories(include)

# Find threading library
find_package(Threads REQUIRED)

# Source files for library
file(GLOB_RECURSE LIB_SOURCES "src/**/*.cpp")

# Create library
if(LIB_SOURCES)
    add_library(mc_engine ${LIB_SOURCES})
    target_link_libraries(mc_engine Threads::Threads)
endif()

# Example Executables

# Example 1: Basic Pricing
add_executable(basic_pricing examples/basic_pricing.cpp)
target_link_libraries(basic_pricing Threads::Threads)

# Example 2: Validation
add_executable(validation examples/validation.cpp)
target_link_libraries(validation Threads::Threads)

# Example 3: Convergence Study
add_executable(convergence_study examples/convergence_study.cpp)
target_link_libraries(convergence_study Threads::Threads)

# Example 4: Control Variates Demo
add_executable(control_variates_demo examples/control_variates_demo.cpp)
target_link_libraries(control_variates_demo Threads::Threads)

# Example 5: Greeks Comparison
add_executable(greeks_comparison examples/greeks_comparison.cpp)
target_link_libraries(greeks_comparison Threads::Threads)

# Example 6: Performance Benchmark
add_executable(performance_benchmark examples/performance_benchmark.cpp)
target_link_libraries(performance_benchmark Threads::Threads)

# Example 7: asian vs european 
add_executable(asian_vs_european examples/asian_vs_european.cpp)
target_link_libraries(asian_vs_european Threads::Threads)

# Print Configuration
message(STATUS "Monte Carlo Engine Configuration")
message(STATUS "")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "")