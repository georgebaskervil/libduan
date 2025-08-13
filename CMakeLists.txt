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
)
add_library(duansalgorithm::cpp_starter ALIAS cpp_starter)

target_include_directories(cpp_starter
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(cpp_starter PUBLIC cxx_std_20)

enable_project_warnings(cpp_starter)

# Executable
add_executable(duansalgorithm
  src/main.cpp
)

target_link_libraries(duansalgorithm PRIVATE duansalgorithm::cpp_starter)

enable_project_warnings(duansalgorithm)

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
  )
  target_link_libraries(cpp_starter_tests PRIVATE
    duansalgorithm::cpp_starter
    GTest::gtest_main
  )
  target_compile_features(cpp_starter_tests PRIVATE cxx_std_20)
  enable_project_warnings(cpp_starter_tests)

  include(GoogleTest)
  gtest_discover_tests(cpp_starter_tests)
endif()
