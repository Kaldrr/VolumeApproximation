include_guard(GLOBAL)

#[[
Debug-only sanitizers.

project_options' own sanitizer switches aren't used because they:
  * are decided once at configure time instead of per configuration, and
  * pass -fsanitize=... to every language, which nvcc rejects.

This module creates the INTERFACE target `<prefix>_sanitizers`, which adds the
selected sanitizers only to Debug builds:
  * C++ sources get -fsanitize=...
  * CUDA sources get the flags forwarded to the host compiler (-Xcompiler)
  * host links get -fsanitize=...

ASan and TSan can't be combined in one binary (both need their own shadow
memory), so pick one per build directory with DEBUG_SANITIZERS.
]]

set(DEBUG_SANITIZERS
    "address;undefined"
    CACHE STRING
          "Sanitizers for Debug builds (address, undefined, leak, thread). Use 'thread;undefined' for TSan, empty to disable."
)
set_property(CACHE DEBUG_SANITIZERS PROPERTY STRINGS "address;undefined" "thread;undefined" "")

set(_VA_KNOWN_SANITIZERS address undefined leak thread)

function(volume_approximation_sanitizers prefix)
  set(target ${prefix}_sanitizers)
  add_library(${target} INTERFACE)

  set(sanitizers ${DEBUG_SANITIZERS})
  if(NOT sanitizers)
    return()
  endif()

  foreach(sanitizer IN LISTS sanitizers)
    if(NOT sanitizer IN_LIST _VA_KNOWN_SANITIZERS)
      message(FATAL_ERROR "DEBUG_SANITIZERS: unknown sanitizer '${sanitizer}'. Known: ${_VA_KNOWN_SANITIZERS}")
    endif()
  endforeach()
  if("thread" IN_LIST sanitizers AND ("address" IN_LIST sanitizers OR "leak" IN_LIST sanitizers))
    message(FATAL_ERROR "DEBUG_SANITIZERS: 'thread' can't be combined with 'address' or 'leak'. "
                        "Use a separate build directory, e.g. -DDEBUG_SANITIZERS=\"thread;undefined\".")
  endif()

  if(MSVC)
    # MSVC only provides ASan
    if("address" IN_LIST sanitizers)
      target_compile_options(${target} INTERFACE "$<$<AND:$<CONFIG:Debug>,$<COMPILE_LANGUAGE:CXX>>:/fsanitize=address>")
      message(STATUS "Debug sanitizers: address (MSVC supports no others)")
    endif()
    return()
  endif()

  # One -fsanitize=<name> per sanitizer: commas would be split both by generator
  # expressions and by nvcc's -Xcompiler
  set(flags -fno-omit-frame-pointer)
  foreach(sanitizer IN LISTS sanitizers)
    list(APPEND flags -fsanitize=${sanitizer})
  endforeach()
  if("undefined" IN_LIST sanitizers)
    # Abort with a stack trace on UB, instead of continuing
    list(APPEND flags -fno-sanitize-recover=undefined)
  endif()
  # One generator expression per flag (lists inside them get mangled for CUDA)
  set(is_debug "$<CONFIG:Debug>")
  foreach(flag IN LISTS flags)
    target_compile_options(
      ${target} INTERFACE "$<$<AND:${is_debug},$<COMPILE_LANGUAGE:CXX>>:${flag}>"
                          "$<$<AND:${is_debug},$<COMPILE_LANGUAGE:CUDA>>:-Xcompiler=${flag}>")
    target_link_options(${target} INTERFACE "$<${is_debug}:$<HOST_LINK:${flag}>>")
  endforeach()

  list(JOIN sanitizers "," pretty)
  message(STATUS "Debug sanitizers: ${pretty}")
endfunction()
