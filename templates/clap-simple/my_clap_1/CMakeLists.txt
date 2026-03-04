project(MyClap1)

add_library(${PROJECT_NAME} SHARED main.cpp clap_wrapper.cpp)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    OUTPUT_NAME "MyClap1"
    POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(${PROJECT_NAME} PRIVATE sonic_common)