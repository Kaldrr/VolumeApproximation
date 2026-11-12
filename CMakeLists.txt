cmake_minimum_required(VERSION 3.30)

project(VolumeApproximation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

option(CUDA_ENABLED "Build the optional CUDA backend" OFF)
option(FEATURE_CHECKS "Run static analysis (clang-tidy with Clang)" ON)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/project_options/CMakeLists.txt")
  message(FATAL_ERROR "project_options submodule is missing. Run: git submodule update --init")
endif()
add_subdirectory(project_options)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Sanitizers)

set(ENABLE_CLANG_TIDY "")
if(FEATURE_CHECKS AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # GCC sets some module specific flags which upset clang-tidy
  # (-fdeps-format=p1689r5, -fmodule-mapper and -fmodule-ts), so only use it with Clang
  set(ENABLE_CLANG_TIDY ENABLE_CLANG_TIDY)
endif()

project_options(
  PREFIX
  "VolumeApproximation"
  ${ENABLE_CLANG_TIDY}
  ENABLE_INTERPROCEDURAL_OPTIMIZATION
  ENABLE_NATIVE_OPTIMIZATION
  # WARNINGS_AS_ERRORS: enable once GCC stops warning about Qt-generated code
)

# Creates VolumeApproximation_sanitizers (Debug-only, see DEBUG_SANITIZERS)
volume_approximation_sanitizers(VolumeApproximation)

add_subdirectory(ApproximationLib)
add_subdirectory(UserInterface)
