cmake_minimum_required(VERSION 3.14.0)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15 CACHE STRING "")

project(Project1
    VERSION 1.0.0.0
    DESCRIPTION "Project1 VST 3 Plug-in"
)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
  add_definitions(-DDEVELOPMENT=1)
else()
  add_definitions(-DRELEASE=1)
endif()

if(APPLE)
    enable_language(OBJCXX)
endif()

if(NOT FETCHCONTENT_BASE_DIR)
  #store fetchContent cache in ~/.sonic/deps
  set(FETCHCONTENT_BASE_DIR "$ENV{HOME}/.sonic/deps" CACHE PATH "" FORCE)
endif()

option(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES "Enable VST 3 Plug-in Examples" OFF)
option(SMTG_ENABLE_VST3_HOSTING_EXAMPLES "Enable VST 3 Hosting Examples" OFF)
option(SMTG_ENABLE_VSTGUI_SUPPORT "Enable VSTGUI Support" OFF)


set(SONIC_GIT_REPOSITORY https://github.com/yahiro07/sonic.git)
set(SONIC_GIT_COMMIT_HASH "080de34ccda2c7b1ff0b8b7dc306d7a7011abf27")
include(FetchContent)
#----------------
#fetch vst3sdk (stripped down version)
FetchContent_Declare(
    sonic_vst3sdk
    GIT_REPOSITORY ${SONIC_GIT_REPOSITORY}
    GIT_TAG ${SONIC_GIT_COMMIT_HASH}
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR  dependencies/vst3sdk
)
FetchContent_MakeAvailable(sonic_vst3sdk)
#----------------

# Make VST3 SDK root path available to subprojects.
smtg_enable_vst3_sdk()
get_filename_component(SMTG_VST3_LOCATION_DIR "${public_sdk_SOURCE_DIR}" DIRECTORY)
set(SDK_ROOT "${SMTG_VST3_LOCATION_DIR}")

#一旦構成の簡単さを優先して重複してgitリポジトリをフェッチする
#vst3sdk, VstDevHost, sonic の3つのコードで重複してフェッチが発生するのであとで改善したい

#----------------
#fetch VstDevHost
FetchContent_Declare(
    sonic_vst_dev_host
    GIT_REPOSITORY ${SONIC_GIT_REPOSITORY}
    GIT_TAG ${SONIC_GIT_COMMIT_HASH}
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR  templates/VstDevHost/vst_dev_host
)
FetchContent_MakeAvailable(sonic_vst_dev_host)
#----------------
#fetch sonic vst wrapper (or use local code)
if(NOT USE_LOCAL_SONIC)
  FetchContent_Declare(
      sonic_vst_wrapper
      GIT_REPOSITORY ${SONIC_GIT_REPOSITORY}
      GIT_TAG ${SONIC_GIT_COMMIT_HASH}
      GIT_SHALLOW TRUE
      SOURCE_SUBDIR  templates/vst-simple/sonic
  )
  FetchContent_MakeAvailable(sonic_vst_wrapper)
else()
  add_subdirectory("sonic")
endif()
#----------------

# option(SONIC_FORCE_FETCHED_VSTDEVHOST "Force using VstDevHost from fetched sonic sources" OFF)

# set(_local_vstdevhost_dir "${CMAKE_CURRENT_SOURCE_DIR}/vst_dev_host")

# if(EXISTS "${_local_vstdevhost_dir}/CMakeLists.txt" AND NOT SONIC_FORCE_FETCHED_VSTDEVHOST)
#     add_subdirectory("vst_dev_host")
# else()
#     # Use the already fetched sonic repo (via sonic_vst3sdk) as a source for VstDevHost.
#     FetchContent_GetProperties(sonic_vst3sdk)
#     if(NOT sonic_vst3sdk_POPULATED)
#         message(FATAL_ERROR "sonic_vst3sdk is not populated. Ensure FetchContent_MakeAvailable(sonic_vst3sdk) ran before adding VstDevHost.")
#     endif()

#     set(_vstdevhost_candidates
#         "${sonic_vst3sdk_SOURCE_DIR}/templates/vst-simple/vst_dev_host"
#         "${sonic_vst3sdk_SOURCE_DIR}/templates/VstDevHost/vst_dev_host"
#     )
#     get_filename_component(_sonic_parent "${sonic_vst3sdk_SOURCE_DIR}" DIRECTORY)
#     list(APPEND _vstdevhost_candidates
#         "${_sonic_parent}/templates/vst-simple/vst_dev_host"
#         "${_sonic_parent}/templates/VstDevHost/vst_dev_host"
#     )
#     get_filename_component(_sonic_parent2 "${_sonic_parent}" DIRECTORY)
#     list(APPEND _vstdevhost_candidates
#         "${_sonic_parent2}/templates/vst-simple/vst_dev_host"
#         "${_sonic_parent2}/templates/VstDevHost/vst_dev_host"
#     )

#     set(VSTDEVHOST_SOURCE_DIR "")
#     foreach(_cand IN LISTS _vstdevhost_candidates)
#         if(EXISTS "${_cand}/CMakeLists.txt")
#             set(VSTDEVHOST_SOURCE_DIR "${_cand}")
#             break()
#         endif()
#     endforeach()

#     if(VSTDEVHOST_SOURCE_DIR STREQUAL "")
#         message(FATAL_ERROR "Could not find VstDevHost sources. Expected either a local ./vst_dev_host/ or one of: ${_vstdevhost_candidates}")
#     endif()

#     add_subdirectory("${VSTDEVHOST_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/vstdevhost_build")
# endif()

#----------------


smtg_add_vst3plugin(Project1
    source/version.h
    source/plugin_factory.cpp
    source/project1_synthesizer.cpp
)

smtg_target_configure_version_file(Project1)

target_link_libraries(Project1
    PRIVATE
        sonic
)

#----------------
#build frontend

if(SONIC_BUILD_FRONTEND)
  execute_process(
      COMMAND npm install
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/frontend"
      RESULT_VARIABLE NODE_BUILD_RESULT
  )

  execute_process(
      COMMAND npm run build
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/frontend"
      RESULT_VARIABLE NODE_BUILD_RESULT
  )

  if(NOT NODE_BUILD_RESULT EQUAL 0)
      message(FATAL_ERROR "frontend project build failed with exit code ${NODE_BUILD_RESULT}")
  endif()
endif()
#----------------


#----------------
#copy html page assets
# Keep web resources in sync with the built plug-in bundle.
# This registers every file under resource/www (recursively) as a plug-in resource
# and preserves the folder layout under <Bundle>/Contents/Resources/www/...
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
#----------------


if(SMTG_MAC)
    smtg_target_set_bundle(Project1
        BUNDLE_IDENTIFIER com.mycompany.project1
        COMPANY_NAME "MyCompany"
    )
    smtg_target_set_debug_executable(Project1
        "/Applications/VST3PluginTestHost.app"
        "--pluginfolder;$(BUILT_PRODUCTS_DIR)"
    )

elseif(SMTG_WIN)
    target_sources(Project1 PRIVATE
        resource/win32resource.rc
    )
    if(MSVC)
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Project1)
        smtg_target_set_debug_executable(Project1
            "$(ProgramW6432)/Steinberg/VST3PluginTestHost/VST3PluginTestHost.exe"
            "--pluginfolder \"$(OutDir)/\""
        )
    endif()
endif()
