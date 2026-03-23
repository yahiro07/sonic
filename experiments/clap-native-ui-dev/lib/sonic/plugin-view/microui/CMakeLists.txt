add_library(plugin-view-microui STATIC)

target_sources(plugin-view-microui PRIVATE microui.c window-representor.mm
                                           microui-editor.cpp)

target_include_directories(plugin-view-microui
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(plugin-view-microui PUBLIC sonic-headers)

target_link_libraries(plugin-view-microui PRIVATE "-framework Cocoa"
                                                  "-framework CoreGraphics")
