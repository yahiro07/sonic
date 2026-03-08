cmake_minimum_required(VERSION 3.20)

if(APPLE)
	set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
endif()

project(project1)

add_subdirectory(plugin)

add_subdirectory(lib/wrapper-auv3)

