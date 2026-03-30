set(SONIC_PUBLIC_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
add_library(sonic-headers INTERFACE)
target_include_directories(sonic-headers INTERFACE ${SONIC_PUBLIC_INCLUDE_DIR})

if(XCODE)
  add_subdirectory(platform/auv3)
else()
  add_subdirectory(platform/vst3)
  add_subdirectory(platform/clap)
endif()

add_subdirectory(plugin-view/webview)
