cmake_minimum_required(VERSION 3.14)

project(clap_template LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)
FetchContent_Declare(
    clap
    GIT_REPOSITORY https://github.com/free-audio/clap.git
    GIT_TAG main
)
FetchContent_MakeAvailable(clap)

include_directories(${clap_SOURCE_DIR}/include)

add_subdirectory(sonic_common)

add_subdirectory(my_clap_1)




