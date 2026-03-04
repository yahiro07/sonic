project(sonic_common)

add_library(${PROJECT_NAME} STATIC
  general/logger.cpp 
  logic/parameter_builder_impl.cpp
  logic/parameter_definitions_provider.cpp
)

target_link_libraries(${PROJECT_NAME} PUBLIC sdk)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# include(FetchContent)
# FetchContent_Declare(
#   glaze
#   GIT_REPOSITORY https://github.com/stephenberry/glaze.git
#   GIT_TAG v7.1.0
#   GIT_SHALLOW TRUE
# )
# FetchContent_MakeAvailable(glaze)
# target_link_libraries(${PROJECT_NAME} PRIVATE glaze::glaze)


if(SMTG_MAC)
  set_source_files_properties(general/mac_web_view.mm PROPERTIES LANGUAGE CXX)

  target_sources(${PROJECT_NAME} PRIVATE general/mac_web_view.mm)

    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework Cocoa"
    "-framework WebKit"
    )
endif()