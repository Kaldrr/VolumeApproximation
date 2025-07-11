cmake_minimum_required(VERSION 4.0.2)

project(VolumeApproximation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)

if(WIN32)
  set(CMAKE_INCLUDE_CURRENT_DIR ON)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(project_options)

option(FEATURE_CHECKS "Enable more checks + sanitizers" ON)
# Enable sanitizers and static analyzers
if(FEATURE_CHECKS)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # GCC sets some module specific flags which upset clang-tidy
    # -fdeps-format=p1689r5, -fmodule-mapper and -fmodule-ts
    set(ENABLE_CLANG_TIDY "ENABLE_CLANG_TIDY")
  endif()
  #

#  set(ENABLE_SANITIZER_ADDRESS "ENABLE_SANITIZER_ADDRESS")
#  set(ENABLE_SANITIZER_UNDEFINED "ENABLE_SANITIZER_UNDEFINED")
  # set(ENABLE_SANITIZER_MEMORY "ENABLE_SANITIZER_MEMORY")
endif()

# Initialize project_options variable related to this project This overwrites
# `project_options` and sets `project_warnings` uncomment to enable the options.
# Some of them accept one or more inputs:
project_options(
  PREFIX
  "VolumeApproximation"
  ${ENABLE_CPPCHECK}
  ${CPPCHECK_OPTIONS}
  ${ENABLE_CLANG_TIDY}
  ENABLE_VS_ANALYSIS
  ENABLE_INTERPROCEDURAL_OPTIMIZATION
  ENABLE_NATIVE_OPTIMIZATION
  ${ENABLE_SANITIZER_ADDRESS}
  ${ENABLE_SANITIZER_UNDEFINED}
  # ${ENABLE_SANITIZER_THREAD}
  ${ENABLE_SANITIZER_MEMORY}
  # ENABLE_SANITIZER_POINTER_COMPARE ENABLE_SANITIZER_POINTER_SUBTRACT
  # ENABLE_CONTROL_FLOW_PROTECTION ENABLE_STACK_PROTECTION
  # ENABLE_OVERFLOW_PROTECTION ENABLE_ELF_PROTECTION
  # ENABLE_RUNTIME_SYMBOLS_RESOLUTION ENABLE_COMPILE_COMMANDS_SYMLINK

  # Would be nice to have, but GCC has some weird Qt-generated-code-issues in
  # release build right now...
  # WARNINGS_AS_ERRORS
  # enable later ENABLE_INCLUDE_WHAT_YOU_USE
)

add_subdirectory(ApproximationLib)
add_subdirectory(UserInterface)
