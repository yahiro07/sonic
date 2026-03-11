cmake_minimum_required(VERSION 3.20)
project(project1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(APPLE)
	set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
endif()

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

add_subdirectory(plugin)

add_subdirectory(lib/platform/clap)
add_subdirectory(targets/clap)

# add_subdirectory(lib/platform/vst3)
# add_subdirectory(targets/vst3)

# add_subdirectory(lib/wrapper-auv3)




