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

execute_process(COMMAND cp -r "../_common/pages" "${CMAKE_CURRENT_SOURCE_DIR}")

# build frontend
set(FRONTEND_OUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/pages/www-bundles")
execute_process(
  COMMAND sh -c "npm install && npx vite build --outDir ${FRONTEND_OUT_DIR}"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../_common/frontend"
  RESULT_VARIABLE NODE_BUILD_RESULT)

if(NOT NODE_BUILD_RESULT EQUAL 0)
  message(
    FATAL_ERROR
      "frontend project build failed with exit code ${NODE_BUILD_RESULT}")
endif()

option(SONIC_USE_LOCAL_SDK "Use vendored SDKs under dependencies/" ON)
if(SONIC_USE_LOCAL_SDK)
  include(cmake/setup-sdks-dev.cmake)
  add_subdirectory(framework/sonic)
else()
  include(cmake/setup-sdks.cmake)
  add_subdirectory(
    "${SONIC_ROOT_DIR}/templates/cpp-multi-target/framework/sonic"
    "${CMAKE_CURRENT_BINARY_DIR}/framework/sonic")
endif()

add_subdirectory(${SONIC_ROOT_DIR}/hosts/vst-dev-host/vst_dev_host
                 ${CMAKE_CURRENT_BINARY_DIR}/vst_dev_host)
add_subdirectory(${SONIC_ROOT_DIR}/hosts/clap-dev-host/clap_dev_host
                 ${CMAKE_CURRENT_BINARY_DIR}/clap_dev_host)

add_subdirectory(source)

add_subdirectory(wrapper/vst3)
add_subdirectory(wrapper/clap)
