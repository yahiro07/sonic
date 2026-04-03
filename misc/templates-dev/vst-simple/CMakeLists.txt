# ----------------
# basic setup

cmake_minimum_required(VERSION 3.14.0)
set(CMAKE_OSX_DEPLOYMENT_TARGET
    10.15
    CACHE STRING "")

project(
  Project1
  VERSION 1.0.0.0
  DESCRIPTION "Project1 VST 3 Plug-in")

if(CMAKE_BUILD_TYPE MATCHES "Debug")
  add_definitions(-DDEVELOPMENT=1)
else()
  add_definitions(-DRELEASE=1)
endif()

if(APPLE)
  enable_language(OBJCXX)
endif()

# ----------------
# fetch dependent modules

if(NOT FETCHCONTENT_BASE_DIR)
  # store fetchContent cache in ~/.sonic/deps
  set(FETCHCONTENT_BASE_DIR
      "$ENV{HOME}/.sonic/deps"
      CACHE PATH "" FORCE)
endif()

# fetch sonic repository root
include(FetchContent)
FetchContent_Populate(
  sonic_repo
  GIT_REPOSITORY https://github.com/yahiro07/sonic.git
  GIT_TAG "f13855e9a1f6fea4e2208b427964d0b0d7224574"
  GIT_SHALLOW TRUE
  GIT_SUBMODULES_RECURSE YES)

set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF)
set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF)
set(SMTG_ENABLE_VSTGUI_SUPPORT OFF)
set(SMTG_RUN_VST_VALIDATOR OFF)

# add vst3sdk
add_subdirectory(${sonic_repo_SOURCE_DIR}/dependencies/vst3sdk)

# Make VST3 SDK root path available to subprojects.
smtg_enable_vst3_sdk()
get_filename_component(SMTG_VST3_LOCATION_DIR "${public_sdk_SOURCE_DIR}"
                       DIRECTORY)
set(SDK_ROOT "${SMTG_VST3_LOCATION_DIR}")

# add VstDevHost
add_subdirectory(${sonic_repo_SOURCE_DIR}/templates/VstDevHost/vst_dev_host)

# add sonic vst3 wrapper library
if(USE_LOCAL_SONIC)
  add_subdirectory("sonic")
else()
  add_subdirectory(${sonic_repo_SOURCE_DIR}/templates/vst-simple/sonic)
endif()

# ----------------
# plugin source configurations

# if(VSTDEVHOST_SOURCE_DIR STREQUAL "") message(FATAL_ERROR "Could not find
# VstDevHost sources. Expected either a local ./vst_dev_host/ or one of:
# ${_vstdevhost_candidates}") endif()

# add_subdirectory("${VSTDEVHOST_SOURCE_DIR}"
# "${CMAKE_BINARY_DIR}/vstdevhost_build") endif()

# ----------------

smtg_add_vst3plugin(Project1 source/version.h source/plugin_factory.cpp
                    source/project1_synthesizer.cpp)

smtg_target_configure_version_file(Project1)

target_link_libraries(Project1 PRIVATE sonic)

execute_process(
  COMMAND npm run build
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/frontend"
  RESULT_VARIABLE NODE_BUILD_RESULT)

if(NOT NODE_BUILD_RESULT EQUAL 0)
  message(
    FATAL_ERROR
      "frontend project build failed with exit code ${NODE_BUILD_RESULT}")
endif()

# ----------------
# copy html page assets

# Keep web resources in sync with the built plug-in bundle. This registers every
# file under resource/www (recursively) as a plug-in resource and preserves the
# folder layout under <Bundle>/Contents/Resources/www/...
set(PLUGIN_WWW_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/resource/www")
file(GLOB_RECURSE PLUGIN_WWW_FILES CONFIGURE_DEPENDS "${PLUGIN_WWW_ROOT}/*")
foreach(_www_file IN LISTS PLUGIN_WWW_FILES)
  if(IS_DIRECTORY "${_www_file}")
    continue()
  endif()

  file(RELATIVE_PATH _www_relpath "${PLUGIN_WWW_ROOT}" "${_www_file}")
  get_filename_component(_www_reldir "${_www_relpath}" DIRECTORY)

  if(_www_reldir STREQUAL "")
    set(_www_outsubdir "www")
  else()
    set(_www_outsubdir "www/${_www_reldir}")
  endif()

  smtg_target_add_plugin_resource(Project1 "${_www_file}" "${_www_outsubdir}")
endforeach()

# ----------------
# output configurations

if(SMTG_MAC)
  smtg_target_set_bundle(Project1 BUNDLE_IDENTIFIER com.mycompany.project1
                         COMPANY_NAME "MyCompany")
  smtg_target_set_debug_executable(
    Project1 "/Applications/VST3PluginTestHost.app"
    "--pluginfolder;$(BUILT_PRODUCTS_DIR)")

elseif(SMTG_WIN)
  target_sources(Project1 PRIVATE resource/win32resource.rc)
  if(MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                 PROPERTY VS_STARTUP_PROJECT Project1)
    smtg_target_set_debug_executable(
      Project1
      "$(ProgramW6432)/Steinberg/VST3PluginTestHost/VST3PluginTestHost.exe"
      "--pluginfolder \"$(OutDir)/\"")
  endif()
endif()

# ----------------
