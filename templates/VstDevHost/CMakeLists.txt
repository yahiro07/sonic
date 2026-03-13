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

# ----------------
# fetch vst3sdk
option(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES "Enable VST 3 Plug-in Examples" OFF)
option(SMTG_ENABLE_VST3_HOSTING_EXAMPLES "Enable VST 3 Hosting Examples" OFF)
option(SMTG_ENABLE_VSTGUI_SUPPORT "Enable VSTGUI Support" OFF)

if(NOT SONIC_USE_LOCAL_SDK)
  include(FetchContent)
  FetchContent_Populate(
    sonic_repo
    GIT_REPOSITORY https://github.com/yahiro07/sonic.git
    GIT_TAG "f13855e9a1f6fea4e2208b427964d0b0d7224574"
    GIT_SHALLOW TRUE
    GIT_SUBMODULES_RECURSE YES)
else()
  get_filename_component(
    VST3SDK_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../dependencies/vst3sdk"
    ABSOLUTE)
  add_subdirectory("${VST3SDK_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/vst3sdk_build")
endif()

smtg_enable_vst3_sdk()
get_filename_component(SMTG_VST3_LOCATION_DIR "${public_sdk_SOURCE_DIR}"
                       DIRECTORY)
set(SDK_ROOT "${SMTG_VST3_LOCATION_DIR}")

# ----------------

add_subdirectory(vst_dev_host)
