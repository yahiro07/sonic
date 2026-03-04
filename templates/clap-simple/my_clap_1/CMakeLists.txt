add_library(MyClap1 SHARED main.cpp clap_wrapper.cpp)

set_target_properties(MyClap1 PROPERTIES
    PREFIX ""
    OUTPUT_NAME "MyClap1"
    POSITION_INDEPENDENT_CODE ON
)