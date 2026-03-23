add_library(native-ui-microui STATIC)

target_sources(native-ui-microui PRIVATE microui-editor-view.cpp)

target_include_directories(native-ui-microui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(native-ui-microui PUBLIC plugin-view-microui)
