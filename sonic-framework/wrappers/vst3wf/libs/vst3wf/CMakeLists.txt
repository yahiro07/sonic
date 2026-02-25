cmake_minimum_required(VERSION 3.14)
project(vst3wf)

add_library(vst3wf STATIC greet.cpp logger.cpp)

set_target_properties(vst3wf PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)