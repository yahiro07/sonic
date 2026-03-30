cmake_minimum_required(VERSION 3.14.0)
project(VstDevHost_Root)

if(APPLE)
  enable_language(OBJC)
  enable_language(OBJCXX)
endif()

if(CMAKE_BUILD_TYPE MATCHES "Debug")
  add_definitions(-DDEVELOPMENT=1)
else()
  add_definitions(-DRELEASE=1)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(SONIC_USE_LOCAL_SDK ON)

# ----------------
# setup vst3sdk

set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF)
set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF)
set(SMTG_ENABLE_VSTGUI_SUPPORT OFF)
set(SMTG_RUN_VST_VALIDATOR OFF)

if(NOT SONIC_USE_LOCAL_SDK)
  include(FetchContent)
  set(FETCHCONTENT_QUIET OFF)
  FetchContent_Declare(
    vst3sdk
    GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk
    GIT_TAG v3.8.0_build_66
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
    UPDATES_DISCONNECTED TRUE)
  FetchContent_MakeAvailable(vst3sdk)
else()
  get_filename_component(
    VST3SDK_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../dependencies/vst3sdk"
    ABSOLUTE)
  add_subdirectory("${VST3SDK_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/vst3sdk_build")
endif()

smtg_enable_vst3_sdk()
get_filename_component(SMTG_VST3_LOCATION_DIR "${public_sdk_SOURCE_DIR}"
                       DIRECTORY)
set(VST3_SDK_ROOT "${SMTG_VST3_LOCATION_DIR}")

# ----------------

option(SONIC_DEBUG_LOGS "Enable logging" OFF)
option(SONIC_DEBUG_USE_UDP_LOGGER "Enable UDP logging" OFF)
if(SONIC_DEBUG_LOGS)
  add_compile_definitions(SONIC_DEBUG_LOGS)
endif()
if(SONIC_DEBUG_USE_UDP_LOGGER)
  add_compile_definitions(SONIC_DEBUG_USE_UDP_LOGGER)
endif()

add_subdirectory(vst_dev_host)
