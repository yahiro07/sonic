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

project(project1 LANGUAGES C CXX OBJCXX)

include(cmake/CppModulesSupport.cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
