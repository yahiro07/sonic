add_library(native-ui-microui STATIC)

target_sources(native-ui-microui PRIVATE microui-editor-view.cpp)

target_include_directories(native-ui-microui
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
