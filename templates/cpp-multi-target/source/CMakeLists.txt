set(NAME plugin-source)
project(${NAME} LANGUAGES CXX)

add_library(${NAME} STATIC)

target_sources(${NAME} PRIVATE project1-synthesizer.cpp project1-factory.cpp)

target_link_libraries(${NAME} PRIVATE sonic-headers plugin-view-webview)
