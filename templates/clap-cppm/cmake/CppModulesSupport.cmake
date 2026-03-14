# message(STATUS "CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}") message(STATUS
# "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}") message(STATUS
# "CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}") set(CMAKE_C_COMPILER
# "/opt/homebrew/opt/llvm/bin/clang") set(CMAKE_CXX_COMPILER
# "/opt/homebrew/opt/llvm/bin/clang++") message(↓) message(STATUS
# "CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}") message(STATUS "CMAKE_CXX_COMPILER:
# ${CMAKE_CXX_COMPILER}")

# C++20 Modules (CMake) need a dependency scanner.
if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  message(
    FATAL_ERROR
      "C++20 Modules require clang-scan-deps; use LLVM Clang (e.g. 'brew install llvm')."
  )
endif()

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  message(
    FATAL_ERROR
      "Expecting Compiler ID Clang, but found ${CMAKE_CXX_COMPILER_ID}")
endif()

find_program(_clang_scan_deps NAMES clang-scan-deps)
if(NOT _clang_scan_deps)
  message(
    FATAL_ERROR
      "C++20 Modules require clang-scan-deps (not found). Install an LLVM Clang toolchain that provides it."
  )
endif()

# macOS + upstream LLVM/Clang: make sure the SDK sysroot is set so system
# headers (e.g. assert.h) can be found.
if(APPLE AND NOT DEFINED CMAKE_OSX_SYSROOT)
  execute_process(
    COMMAND xcrun --sdk macosx --show-sdk-path
    OUTPUT_VARIABLE _macos_sdk
    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
  if(EXISTS "${_macos_sdk}")
    set(CMAKE_OSX_SYSROOT
        "${_macos_sdk}"
        CACHE PATH "macOS SDK sysroot" FORCE)
  endif()
endif()
