# Project: duansalgorithm
# License: Placeholder - add your license details here.

cmake_minimum_required(VERSION 3.20)
project(duansalgorithm VERSION 0.1.0 LANGUAGES CXX)

# Global C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Silence FetchContent timestamp warning when supported
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

# Output dirs
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Options
option(ENABLE_WERROR "Treat warnings as errors" OFF)
option(ENABLE_MIMALLOC "Link against mimalloc if available" ON)
option(ENABLE_GLOBAL_MIMALLOC "Override global new/delete using mimalloc" OFF)
option(ENABLE_MIMALLOC_DIAGNOSTICS "Print mimalloc version and stats at startup" OFF)
option(ENABLE_BMSSP_VERIFIER "Enable BMSSP invariant checking and verbose stats" OFF)
option(CONST_DEGREE_STRICT_MODE "Abort if input graph degree > 2 (requires ENABLE_BMSSP_VERIFIER)" OFF)
set(MIN_DISTANCE_BITS "64" CACHE STRING "Minimum distance bit width for BMSSP distances (32 or 64)")
set_property(CACHE MIN_DISTANCE_BITS PROPERTY STRINGS 32 64)
option(ENABLE_DISTANCE_WIDENING "Allow automatic distance widening on overflow" ON)
option(ENABLE_BIGINT_FALLBACK "Enable bigint fallback after 128-bit widening (experimental)" OFF)

# Common warnings function
function(enable_project_warnings target_name)
  if(MSVC)
    target_compile_options(${target_name} PRIVATE /W4)
  else()
    target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
      target_compile_options(${target_name} PRIVATE -Wshadow -Wconversion -Wsign-conversion -Wimplicit-fallthrough)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      target_compile_options(${target_name} PRIVATE -Wshadow -Wconversion -Wsign-conversion -Wno-psabi)
    endif()
    if(ENABLE_WERROR)
      target_compile_options(${target_name} PRIVATE -Werror)
    endif()
  endif()
endfunction()

# Library target

add_library(cpp_starter
  src/lib.cpp
  src/bmssp_algo.cpp
  src/base_case.cpp
  src/bmssp.cpp
  src/bmssp_state.cpp
  src/find_pivots.cpp
  src/structure.cpp
  src/instrumentation.cpp
  src/degree_check.cpp
)
add_library(duansalgorithm::cpp_starter ALIAS cpp_starter)

target_include_directories(cpp_starter
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(cpp_starter PUBLIC cxx_std_20)

enable_project_warnings(cpp_starter)
if(ENABLE_BMSSP_VERIFIER)
  target_compile_definitions(cpp_starter PUBLIC ENABLE_BMSSP_VERIFIER)
endif()
if(CONST_DEGREE_STRICT_MODE)
  target_compile_definitions(cpp_starter PUBLIC CONST_DEGREE_STRICT_MODE)
endif()
target_compile_definitions(cpp_starter PUBLIC
  MIN_DISTANCE_BITS=${MIN_DISTANCE_BITS}
  $<$<BOOL:${ENABLE_DISTANCE_WIDENING}>:ENABLE_DISTANCE_WIDENING>
  $<$<BOOL:${ENABLE_BIGINT_FALLBACK}>:ENABLE_BIGINT_FALLBACK>
)

# Executable
add_executable(duansalgorithm
  src/main.cpp
)

target_link_libraries(duansalgorithm PRIVATE duansalgorithm::cpp_starter)

enable_project_warnings(duansalgorithm)

# mimalloc detection and usage (optional)
if(ENABLE_MIMALLOC)
  find_library(MIMALLOC_LIB NAMES mimalloc mimalloc-static)
  find_path(MIMALLOC_INCLUDE_DIR mimalloc.h
    PATHS
      /opt/homebrew/include
      /usr/local/include
      /usr/include
      $ENV{CPATH}
  )
  if(MIMALLOC_LIB)
    # Link publicly on the library so consumers get the allocator symbols.
    target_link_libraries(cpp_starter PUBLIC ${MIMALLOC_LIB})
    target_compile_definitions(cpp_starter PUBLIC HAVE_MIMALLOC)
    if(MIMALLOC_INCLUDE_DIR)
      target_include_directories(cpp_starter PUBLIC ${MIMALLOC_INCLUDE_DIR})
      message(STATUS "mimalloc include: ${MIMALLOC_INCLUDE_DIR}")
    endif()
    message(STATUS "mimalloc found: ${MIMALLOC_LIB}")
    
    # Apply optimized compiler flags for mimalloc builds
    target_compile_options(cpp_starter PRIVATE -O3 -march=native)
    target_compile_options(duansalgorithm PRIVATE -O3 -march=native)
  else()
    message(STATUS "mimalloc not found; using default allocator")
  endif()
endif()

# Optionally override global new/delete using mimalloc (applies to STL by default)
if(ENABLE_GLOBAL_MIMALLOC AND ENABLE_MIMALLOC AND MIMALLOC_LIB)
  target_sources(duansalgorithm PRIVATE src/mimalloc_new_delete.cpp)
endif()

# Testing setup
include(CTest)
if(BUILD_TESTING)
  include(FetchContent)
  # Googletest
  FetchContent_Declare(
    googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  # For MSVC and older compilers, avoid overriding parent CMake vars
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  enable_testing()
  add_executable(cpp_starter_tests
      tests/lib_test.cpp
  tests/structure_test.cpp
  tests/recursion_against_dijkstra_test.cpp
  tests/sssp_driver_test.cpp
  tests/find_pivots_test.cpp
  tests/base_case_test.cpp
  tests/partial_execution_test.cpp
  tests/tie_breaking_test.cpp
  tests/instrumentation_test.cpp
  tests/degree_check_test.cpp
    )
  if(ENABLE_BIGINT_FALLBACK)
    target_sources(cpp_starter_tests PRIVATE tests/bigint_relax_test.cpp)
  endif()
  # Phase 2b tests
  target_sources(cpp_starter_tests PRIVATE tests/boundary_invariants_test.cpp)
  if(ENABLE_BMSSP_VERIFIER)
    target_sources(cpp_starter_tests PRIVATE tests/widen_bench_test.cpp)
  endif()
  target_link_libraries(cpp_starter_tests PRIVATE
    duansalgorithm::cpp_starter
    GTest::gtest_main
  )
  if(ENABLE_BMSSP_VERIFIER)
    target_compile_definitions(cpp_starter_tests PUBLIC ENABLE_BMSSP_VERIFIER)
  endif()
  target_compile_features(cpp_starter_tests PRIVATE cxx_std_20)
  enable_project_warnings(cpp_starter_tests)
  if(ENABLE_GLOBAL_MIMALLOC AND ENABLE_MIMALLOC AND MIMALLOC_LIB)
    target_sources(cpp_starter_tests PRIVATE src/mimalloc_new_delete.cpp)
  endif()

  if(ENABLE_MIMALLOC_DIAGNOSTICS AND ENABLE_MIMALLOC AND MIMALLOC_LIB)
    target_sources(duansalgorithm PRIVATE src/mimalloc_init.cpp)
    target_sources(cpp_starter_tests PRIVATE src/mimalloc_init.cpp)
  endif()

  include(GoogleTest)
  gtest_discover_tests(cpp_starter_tests)
endif()
