cmake_minimum_required(VERSION 3.28)
project(project1 LANGUAGES C CXX OBJCXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(APPLE)
  set(CMAKE_OSX_ARCHITECTURES
      "arm64;x86_64"
      CACHE STRING "" FORCE)
  enable_language(OBJC)
endif()

# ----------------
# setup sdks

# Ninja/Unix Makefiles are single-config; if CMAKE_BUILD_TYPE is empty, VST3 SDK
# (fdebug.h) will error unless one of DEVELOPMENT/RELEASE/_DEBUG/NDEBUG is
# defined. if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
# set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE) set_property(CACHE
# CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo"
# "MinSizeRel") endif() add_compile_definitions(
# $<$<CONFIG:Debug>:DEVELOPMENT=1> $<$<CONFIG:Debug>:_DEBUG=1>
# $<$<CONFIG:Release>:RELEASE=1> $<$<CONFIG:Release>:NDEBUG=1>
# $<$<CONFIG:RelWithDebInfo>:RELEASE=1> $<$<CONFIG:RelWithDebInfo>:NDEBUG=1>
# $<$<CONFIG:MinSizeRel>:RELEASE=1> $<$<CONFIG:MinSizeRel>:NDEBUG=1> )
if(CMAKE_BUILD_TYPE MATCHES "Debug")
  add_definitions(-DDEVELOPMENT=1)
else()
  add_definitions(-DRELEASE=1)
endif()

option(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES "Enable VST 3 Plug-in Examples" OFF)
option(SMTG_ENABLE_VST3_HOSTING_EXAMPLES "Enable VST 3 Hosting Examples" OFF)
option(SMTG_ENABLE_VSTGUI_SUPPORT "Enable VSTGUI Support" OFF)

option(SONIC_USE_LOCAL_SDK "Use vendored SDKs under dependencies/" ON)

if(NOT SONIC_USE_LOCAL_SDK)
  include(FetchContent)

  FetchContent_Declare(
    glaze
    GIT_REPOSITORY https://github.com/stephenberry/glaze.git
    GIT_TAG v7.1.0
    GIT_SHALLOW TRUE
    UPDATES_DISCONNECTED TRUE)

  FetchContent_Declare(
    clap
    GIT_REPOSITORY https://github.com/free-audio/clap.git
    GIT_TAG 1.2.7
    GIT_SHALLOW TRUE
    UPDATES_DISCONNECTED TRUE)

  FetchContent_Declare(
    vst3sdk
    GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk
    GIT_TAG v3.8.0_build_66
    GIT_SHALLOW TRUE
    UPDATES_DISCONNECTED TRUE)

  FetchContent_MakeAvailable(glaze)
  FetchContent_MakeAvailable(clap)
  FetchContent_MakeAvailable(vst3sdk)

else()
  get_filename_component(
    DEPENDENCIES_DIR "${CMAKE_CURRENT_LIST_DIR}/../../dependencies" ABSOLUTE)
  add_subdirectory("${DEPENDENCIES_DIR}/clap" "${CMAKE_BINARY_DIR}/clap_build")
  add_subdirectory("${DEPENDENCIES_DIR}/glaze"
                   "${CMAKE_BINARY_DIR}/glaze_build")
  add_subdirectory("${DEPENDENCIES_DIR}/vst3sdk"
                   "${CMAKE_BINARY_DIR}/vst3sdk_build")
endif()

smtg_enable_vst3_sdk()
get_filename_component(SMTG_VST3_LOCATION_DIR "${public_sdk_SOURCE_DIR}"
                       DIRECTORY)
set(VST3_SDK_ROOT "${SMTG_VST3_LOCATION_DIR}")

# ----------------

add_subdirectory(framework/sonic)

add_subdirectory(plugin)

add_subdirectory(variants/vst3)
add_subdirectory(variants/clap)
