set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")

if(NOT DEFINED CMAKE_CXX_STDLIB_MODULES_JSON
   AND DEFINED CMAKE_CXX_COMPILER
   AND EXISTS "${CMAKE_CXX_COMPILER}")
  execute_process(
    COMMAND "${CMAKE_CXX_COMPILER}" -print-file-name=libc++.modules.json
    OUTPUT_VARIABLE _libcxx_modules_json
    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
  if(IS_ABSOLUTE "${_libcxx_modules_json}" AND EXISTS "${_libcxx_modules_json}")
    set(CMAKE_CXX_STDLIB_MODULES_JSON "${_libcxx_modules_json}")
  else()
    get_filename_component(_cxx_compiler_realpath "${CMAKE_CXX_COMPILER}"
                           REALPATH)
    get_filename_component(_llvm_root "${_cxx_compiler_realpath}" DIRECTORY)
    get_filename_component(_llvm_root "${_llvm_root}" DIRECTORY)
    set(_libcxx_modules_json "${_llvm_root}/lib/c++/libc++.modules.json")
    if(EXISTS "${_libcxx_modules_json}")
      set(CMAKE_CXX_STDLIB_MODULES_JSON "${_libcxx_modules_json}")
    endif()
  endif()
endif()

cmake_minimum_required(VERSION 3.28)

if(APPLE AND "${CMAKE_OSX_SYSROOT}" STREQUAL "")
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

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(project1 LANGUAGES C CXX OBJCXX)

include(cmake/CppModulesSupport.cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

get_filename_component(
  CLAP_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../dependencies/clap" ABSOLUTE)
add_subdirectory("${CLAP_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/clap_build")

get_filename_component(
  GLAZE_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../dependencies/glaze"
  ABSOLUTE)
add_subdirectory("${GLAZE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/glaze_build")

add_subdirectory(plugin)

add_subdirectory(framework)

add_subdirectory(variants/clap)
