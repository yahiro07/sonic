cmake_minimum_required(VERSION 3.28)
project(project1 LANGUAGES C CXX OBJCXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

get_filename_component(
  CLAP_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../dependencies/clap" ABSOLUTE)
add_subdirectory("${CLAP_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/clap_build")

get_filename_component(
  GLAZE_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../dependencies/glaze"
  ABSOLUTE)
add_subdirectory("${GLAZE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/glaze_build")

add_subdirectory(lib/sonic)

add_subdirectory(plugin)

add_subdirectory(native-ui/microui)

add_subdirectory(native-ui/briq)

add_subdirectory(variants/clap)
