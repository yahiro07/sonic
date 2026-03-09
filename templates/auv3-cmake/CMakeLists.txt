cmake_minimum_required(VERSION 3.20)

if(APPLE)
	set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
endif()

project(project1)

include(FetchContent)
FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG v7.1.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glaze)

add_subdirectory(plugin)

add_subdirectory(lib/wrapper-auv3)

