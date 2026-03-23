add_library(plugin-view-briq STATIC)

include(FetchContent)

FetchContent_Declare(
  briq
  GIT_REPOSITORY https://github.com/yahiro07/briq.git
  GIT_TAG main
  GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(briq)

target_sources(plugin-view-briq PRIVATE briq-editor.cpp)

target_include_directories(plugin-view-briq PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(plugin-view-briq PUBLIC sonic-headers briq-lib)
