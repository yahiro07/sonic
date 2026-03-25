get_filename_component(SONIC_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../.."
                       ABSOLUTE)

add_subdirectory("${SONIC_ROOT_DIR}/dependencies/clap"
                 "${CMAKE_CURRENT_BINARY_DIR}/clap")

add_subdirectory("${SONIC_ROOT_DIR}/dependencies/glaze"
                 "${CMAKE_CURRENT_BINARY_DIR}/glaze")

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
