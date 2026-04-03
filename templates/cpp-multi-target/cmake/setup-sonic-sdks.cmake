# Download sonic-sdks repository in ~/.sonic/repos/sonic-sdks/<tag> and share it
# across projects.

set(SONIC_SDKS_REPOSITORY_GIT_TAG "v0.1.0")

if(NOT SONIC_SDKS_REPOSITORY_DIR)
  set(SONIC_SDKS_REPOSITORY_DIR
      "$ENV{HOME}/.sonic/repos/sonic-sdks/${SONIC_SDKS_REPOSITORY_GIT_TAG}"
      CACHE PATH "" FORCE)
endif()

if(EXISTS "${SONIC_SDKS_REPOSITORY_DIR}/.git")
  message(STATUS "use cached sonic-sdks repo: ${SONIC_SDKS_REPOSITORY_DIR}")
else()
  include(FetchContent)
  set(FETCHCONTENT_QUIET OFF)
  FetchContent_Declare(
    sonic_sdks_repo
    GIT_REPOSITORY https://github.com/yahiro07/sonic-sdks.git
    GIT_TAG "${SONIC_SDKS_REPOSITORY_GIT_TAG}"
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
    UPDATES_DISCONNECTED TRUE SOURCE_DIR "${SONIC_SDKS_REPOSITORY_DIR}"
    SOURCE_SUBDIR "")
  FetchContent_MakeAvailable(sonic_sdks_repo)
endif()

set(SONIC_SDKS_ROOT_DIR ${SONIC_SDKS_REPOSITORY_DIR})

add_subdirectory("${SONIC_SDKS_ROOT_DIR}/glaze"
                 "${CMAKE_CURRENT_BINARY_DIR}/glaze")

add_subdirectory("${SONIC_SDKS_ROOT_DIR}/clap"
                 "${CMAKE_CURRENT_BINARY_DIR}/clap")

set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF)
set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF)
set(SMTG_ENABLE_VSTGUI_SUPPORT OFF)
set(SMTG_RUN_VST_VALIDATOR OFF)

add_subdirectory("${SONIC_SDKS_ROOT_DIR}/vst3sdk"
                 "${CMAKE_CURRENT_BINARY_DIR}/vst3sdk")

smtg_enable_vst3_sdk()
get_filename_component(SMTG_VST3_LOCATION_DIR "${public_sdk_SOURCE_DIR}"
                       DIRECTORY)
set(VST3_SDK_ROOT "${SMTG_VST3_LOCATION_DIR}")
