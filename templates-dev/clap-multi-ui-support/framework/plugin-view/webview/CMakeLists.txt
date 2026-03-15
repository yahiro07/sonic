set(NAME plugin-view-webview)

add_library(${NAME} STATIC)

# target_compile_features(${NAME} PRIVATE cxx_std_23)

target_sources(${NAME} PRIVATE mac-web-view.mm webview-bridge.cpp
                               webview-editor.cpp)

set_source_files_properties(mac-web-view.mm PROPERTIES COMPILE_FLAGS
                                                       "-fobjc-arc")

target_link_libraries(${NAME} PRIVATE "-framework Cocoa" "-framework WebKit"
                                      clap glaze::glaze)
