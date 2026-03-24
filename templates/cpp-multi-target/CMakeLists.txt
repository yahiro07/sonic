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

if(CMAKE_BUILD_TYPE MATCHES "Debug")
  add_definitions(-DDEVELOPMENT=1)
else()
  add_definitions(-DRELEASE=1)
endif()

# ----------------
# setup sdks

option(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES "Enable VST 3 Plug-in Examples" OFF)
option(SMTG_ENABLE_VST3_HOSTING_EXAMPLES "Enable VST 3 Hosting Examples" OFF)
option(SMTG_ENABLE_VSTGUI_SUPPORT "Enable VSTGUI Support" OFF)

option(SONIC_USE_LOCAL_SDK "Use vendored SDKs under dependencies/" ON)

if(NOT SONIC_USE_LOCAL_SDK)
  set(SONIC_REPO_GIT_TAG "34b2ce281ca37869fa5c3b4bac37e92dcb0e40e1")

  if(NOT SONIC_REPO_SOURCE_DIR)
    set(SONIC_REPO_SOURCE_DIR
        "$ENV{HOME}/.sonic/repos/${SONIC_REPO_GIT_TAG}/sonic_repo-src"
        CACHE PATH "" FORCE)
  endif()

  if(EXISTS "${SONIC_REPO_SOURCE_DIR}/.git")
    message(STATUS "use cached sonic repo: ${SONIC_REPO_SOURCE_DIR}")
  else()
    include(FetchContent)
    set(FETCHCONTENT_QUIET OFF)
    FetchContent_Declare(
      sonic_repo
      GIT_REPOSITORY https://github.com/yahiro07/sonic.git
      GIT_TAG "${SONIC_REPO_GIT_TAG}"
      GIT_SHALLOW TRUE
      GIT_PROGRESS TRUE
      UPDATES_DISCONNECTED TRUE SOURCE_DIR "${SONIC_REPO_SOURCE_DIR}"
      SOURCE_SUBDIR "experiments")
    FetchContent_MakeAvailable(sonic_repo)
  endif()

  get_filename_component(SONIC_ROOT_DIR "${SONIC_REPO_SOURCE_DIR}" ABSOLUTE)
else()
  get_filename_component(SONIC_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../.."
                         ABSOLUTE)

endif()

add_subdirectory("${SONIC_ROOT_DIR}/dependencies/clap"
                 "${CMAKE_CURRENT_BINARY_DIR}/clap")
add_subdirectory("${SONIC_ROOT_DIR}/dependencies/glaze"
                 "${CMAKE_CURRENT_BINARY_DIR}/glaze")
add_subdirectory("${SONIC_ROOT_DIR}/dependencies/vst3sdk"
                 "${CMAKE_CURRENT_BINARY_DIR}/vst3sdk")

smtg_enable_vst3_sdk()
get_filename_component(SMTG_VST3_LOCATION_DIR "${public_sdk_SOURCE_DIR}"
                       DIRECTORY)
set(VST3_SDK_ROOT "${SMTG_VST3_LOCATION_DIR}")

# ----------------

# add_subdirectory(framework/sonic)
add_subdirectory("${SONIC_ROOT_DIR}/templates/cpp-multi-target/framework/sonic"
                 "${CMAKE_CURRENT_BINARY_DIR}/framework/sonic")

add_subdirectory(plugin)

add_subdirectory(variants/vst3)
add_subdirectory(variants/clap)
