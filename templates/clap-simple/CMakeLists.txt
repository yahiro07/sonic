cmake_minimum_required(VERSION 3.14)

project(clap_template LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# set(FETCHCONTENT_QUIET OFF CACHE BOOL "" FORCE) #debug

include(FetchContent)
FetchContent_Declare(
  clap
  GIT_REPOSITORY https://github.com/free-audio/clap.git
  GIT_TAG 1.2.7
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(clap)
include_directories(${clap_SOURCE_DIR}/include)


include(FetchContent)
FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG v7.1.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glaze)


add_subdirectory(sonic_common)

add_subdirectory(my_clap_1)




