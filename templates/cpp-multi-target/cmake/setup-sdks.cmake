# Download sonic repository in ~/.sonic/repos/<tag> and share it across
# projects.

set(SONIC_REPO_GIT_TAG "v0.1.5")

if(NOT SONIC_REPOSITORY_DIR)
  set(SONIC_REPOSITORY_DIR
      "$ENV{HOME}/.sonic/repos/${SONIC_REPO_GIT_TAG}"
      CACHE PATH "" FORCE)
endif()

if(EXISTS "${SONIC_REPOSITORY_DIR}/.git")
  message(STATUS "use cached sonic repo: ${SONIC_REPOSITORY_DIR}")
else()
  include(FetchContent)
  set(FETCHCONTENT_QUIET OFF)
  FetchContent_Declare(
    sonic_repo
    GIT_REPOSITORY https://github.com/yahiro07/sonic.git
    GIT_TAG "${SONIC_REPO_GIT_TAG}"
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
    UPDATES_DISCONNECTED TRUE SOURCE_DIR "${SONIC_REPOSITORY_DIR}"
    SOURCE_SUBDIR "experiments")
  FetchContent_MakeAvailable(sonic_repo)
endif()

set(SONIC_ROOT_DIR ${SONIC_REPOSITORY_DIR})

add_subdirectory("${SONIC_ROOT_DIR}/dependencies/glaze"
                 "${CMAKE_CURRENT_BINARY_DIR}/glaze")

add_subdirectory("${SONIC_ROOT_DIR}/dependencies/clap"
                 "${CMAKE_CURRENT_BINARY_DIR}/clap")

set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF)
set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF)
set(SMTG_ENABLE_VSTGUI_SUPPORT OFF)
set(SMTG_RUN_VST_VALIDATOR OFF)

add_subdirectory("${SONIC_ROOT_DIR}/dependencies/vst3sdk"
                 "${CMAKE_CURRENT_BINARY_DIR}/vst3sdk")

smtg_enable_vst3_sdk()
get_filename_component(SMTG_VST3_LOCATION_DIR "${public_sdk_SOURCE_DIR}"
                       DIRECTORY)
set(VST3_SDK_ROOT "${SMTG_VST3_LOCATION_DIR}")
