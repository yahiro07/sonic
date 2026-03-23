set(SONIC_PUBLIC_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/..")

add_library(sonic-headers INTERFACE)

target_include_directories(sonic-headers INTERFACE ${SONIC_PUBLIC_INCLUDE_DIR})

add_subdirectory(plugin-view/webview)
add_subdirectory(plugin-view/microui)
add_subdirectory(plugin-view/briq)
add_subdirectory(platform/clap)
