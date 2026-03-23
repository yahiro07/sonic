add_library(plugin-view-briq STATIC)

include(FetchContent)

# FetchContent_Declare( briq GIT_REPOSITORY https://github.com/yahiro07/briq.git
# GIT_TAG main GIT_SHALLOW TRUE UPDATES_DISCONNECTED TRUE)

# for development
get_filename_component(BRIQ_SOURCE_DIR "${CMAKE_SOURCE_DIR}/../../../briq"
                       ABSOLUTE)
FetchContent_Declare(briq SOURCE_DIR ${BRIQ_SOURCE_DIR})

FetchContent_MakeAvailable(briq)

target_sources(plugin-view-briq PRIVATE briq-editor.cpp)

target_include_directories(plugin-view-briq PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(plugin-view-briq PUBLIC sonic-headers briq-lib)
