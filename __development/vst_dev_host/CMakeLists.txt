cmake_minimum_required(VERSION 3.14.0)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15 CACHE STRING "")

project(VstDevHost VERSION 1.0.0.0)

add_executable(VstDevHost main.cpp midi_input_manager_mac.cpp)

set_source_files_properties(app_window_mac.mm PROPERTIES LANGUAGE CXX)

target_sources(VstDevHost PRIVATE app_window_mac.mm)

target_link_libraries(VstDevHost PRIVATE
  "-framework Cocoa"
  "-framework WebKit"
  "-framework CoreMIDI"
  "-framework CoreFoundation"
)