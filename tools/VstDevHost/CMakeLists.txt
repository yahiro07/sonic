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

#----------------
#fetch vst3sdk
option(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES "Enable VST 3 Plug-in Examples" OFF)
option(SMTG_ENABLE_VST3_HOSTING_EXAMPLES "Enable VST 3 Hosting Examples" OFF)
option(SMTG_ENABLE_VSTGUI_SUPPORT "Enable VSTGUI Support" OFF)
include(FetchContent)

if(SMTG_VST3_LOCATION_DIR)
    add_subdirectory(${SMTG_VST3_LOCATION_DIR} ${PROJECT_BINARY_DIR}/vst3sdk_build)
else()
    FetchContent_Declare(
        vst3sdk
        GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
        GIT_TAG v3.8.0_build_66
    )
    FetchContent_MakeAvailable(vst3sdk)
endif()
#----------------

smtg_enable_vst3_sdk()

add_subdirectory(vst_dev_host)

