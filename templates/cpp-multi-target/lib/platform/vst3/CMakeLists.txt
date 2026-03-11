project(wrapper-vst3)

add_library(${PROJECT_NAME} STATIC vst3-entry.cpp)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    OUTPUT_NAME "wrapper-vst3"
    POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(${PROJECT_NAME} PRIVATE plugin glaze::glaze)
