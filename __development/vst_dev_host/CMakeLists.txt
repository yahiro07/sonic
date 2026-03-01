cmake_minimum_required(VERSION 3.14.0)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15 CACHE STRING "")

project(VstDevHost VERSION 1.0.0.0)

add_executable(VstDevHost main.cpp)