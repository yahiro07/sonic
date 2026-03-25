cmake_minimum_required(VERSION 3.28)
project(project1 LANGUAGES C CXX OBJCXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(APPLE)
  set(CMAKE_OSX_ARCHITECTURES
      "arm64;x86_64"
      CACHE STRING "" FORCE)
  enable_language(OBJC)
endif()

if(CMAKE_BUILD_TYPE MATCHES "Debug")
  add_definitions(-DDEVELOPMENT=1)
else()
  add_definitions(-DRELEASE=1)
endif()

include(cmake/setup-sdks.cmake)

add_subdirectory(framework/sonic)
add_subdirectory("${SONIC_ROOT_DIR}/templates/cpp-multi-target/framework/sonic"
                 "${CMAKE_CURRENT_BINARY_DIR}/framework/sonic")

add_subdirectory(${SONIC_ROOT_DIR}/hosts/vst-dev-host/vst_dev_host
                 ${CMAKE_CURRENT_BINARY_DIR}/vst_dev_host)
add_subdirectory(${SONIC_ROOT_DIR}/hosts/clap-dev-host/clap_dev_host
                 ${CMAKE_CURRENT_BINARY_DIR}/clap_dev_host)

add_subdirectory(source)
add_subdirectory(wrapper/vst3)
add_subdirectory(wrapper/clap)
