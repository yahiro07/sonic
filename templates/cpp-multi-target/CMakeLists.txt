cmake_minimum_required(VERSION 3.20)
project(project1 LANGUAGES CXX OBJCXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(APPLE)
	set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
endif()

include(FetchContent)
FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG v7.1.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glaze)

include(FetchContent)
FetchContent_Declare(
  clap
  GIT_REPOSITORY https://github.com/free-audio/clap.git
  GIT_TAG 1.2.7
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(clap)
include_directories(${clap_SOURCE_DIR}/include)

#----------------
#prepare for vst3

if(APPLE)
    enable_language(OBJC)
    enable_language(OBJCXX)
endif()

if(CMAKE_BUILD_TYPE MATCHES "Debug")
  add_definitions(-DDEVELOPMENT=1)
else()
  add_definitions(-DRELEASE=1)
endif()

option(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES "Enable VST 3 Plug-in Examples" OFF)
option(SMTG_ENABLE_VST3_HOSTING_EXAMPLES "Enable VST 3 Hosting Examples" OFF)
option(SMTG_ENABLE_VSTGUI_SUPPORT "Enable VSTGUI Support" OFF)

if(NOT SONIC_USE_LOCAL_VST3SDK)
  include(FetchContent)
  FetchContent_Populate(
      sonic_repo
      GIT_REPOSITORY https://github.com/yahiro07/sonic.git
      GIT_TAG        "8e19c7001a3f130c5621268168ae1feee5b32906"
      GIT_SHALLOW    TRUE
      SOURCE_SUBDIR  dependencies/vst3sdk
  )
else()
  get_filename_component(VST3SDK_SOURCE_DIR
    "${CMAKE_CURRENT_LIST_DIR}/../../dependencies/vst3sdk"
    ABSOLUTE
  )
  add_subdirectory("${VST3SDK_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/vst3sdk_build")
endif()

smtg_enable_vst3_sdk()
get_filename_component(SMTG_VST3_LOCATION_DIR "${public_sdk_SOURCE_DIR}" DIRECTORY)
set(VST3_SDK_ROOT "${SMTG_VST3_LOCATION_DIR}")

#----------------


add_subdirectory(plugin)

# add_subdirectory(lib/platform/clap)
# add_subdirectory(targets/clap)

add_subdirectory(lib/platform/vst3)
add_subdirectory(targets/vst3)

# add_subdirectory(lib/platform/auv3)




