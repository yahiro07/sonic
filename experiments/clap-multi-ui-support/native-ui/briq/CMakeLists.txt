add_library(native-ui-briq STATIC)

target_sources(native-ui-briq PRIVATE briq-editor-view.cpp)

target_include_directories(native-ui-briq PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(native-ui-briq PUBLIC plugin-view-briq)
