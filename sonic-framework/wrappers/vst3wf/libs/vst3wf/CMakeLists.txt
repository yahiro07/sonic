cmake_minimum_required(VERSION 3.14)
project(vst3wf)

add_library(vst3wf STATIC logger.cpp logic/parameter_builder_impl.cpp)

set_target_properties(vst3wf PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

if(SMTG_MAC)
    target_sources(Project1 PRIVATE
        general/mac_web_view.mm
    )

    target_link_libraries(Project1 PRIVATE
        "-framework Cocoa"
        "-framework Webkit"
    )
endif()