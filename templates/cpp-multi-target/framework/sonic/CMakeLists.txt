set(SONIC_PUBLIC_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
add_library(sonic-headers INTERFACE)
target_include_directories(sonic-headers INTERFACE ${SONIC_PUBLIC_INCLUDE_DIR})

add_subdirectory(platform/vst3)
add_subdirectory(platform/clap)
add_subdirectory(platform/auv3)

add_subdirectory(plugin-view/webview)
