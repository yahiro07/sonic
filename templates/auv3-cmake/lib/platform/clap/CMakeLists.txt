project(wrapper-clap)

add_library(${PROJECT_NAME} STATIC clap-entry.cpp)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    OUTPUT_NAME "wrapper-clap"
    POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(${PROJECT_NAME} PRIVATE plugin glaze::glaze)
