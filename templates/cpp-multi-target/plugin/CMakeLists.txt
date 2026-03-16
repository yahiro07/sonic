project(plugin LANGUAGES CXX)

add_library(plugin STATIC)

target_sources(plugin PRIVATE project1-synthesizer.cpp project1-factory.cpp)

target_include_directories(plugin PUBLIC ${CMAKE_CURRENT_LIST_DIR})
