cmake_minimum_required(VERSION 3.14.0)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15 CACHE STRING "")

project(Project1
    VERSION 1.0.0.0
    DESCRIPTION "Project1 VST 3 Plug-in"
)

if(APPLE)
    enable_language(OBJCXX)
endif()

if(NOT FETCHCONTENT_BASE_DIR)
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

# if(USE_LOCAL_SONIC)
add_subdirectory("sonic")
# else()
#     FetchContent_Declare(
#         sonic
#         GIT_REPOSITORY https://github.com/yahiro07/sonic.git
#         GIT_TAG main
#         GIT_SHALLOW TRUE
#         SOURCE_SUBDIR  templates/vst-simple/sonic
#     )
#     FetchContent_MakeAvailable(sonic)
# endif()

smtg_enable_vst3_sdk()

smtg_add_vst3plugin(Project1
    source/version.h
    source/plugin_factory.cpp
    source/project1_synthesizer.cpp
    #--
    # source/vst_basis/plugin_processor.cpp
    # source/vst_basis/plugin_controller.cpp
)

smtg_target_configure_version_file(Project1)


# vst3wf dependency
# - Default: fetch from upstream repository (works for generated project copies that do not include libs/vst3wf).
# - For developing vst3wf itself: configure with -USE_LOCAL_VST3WF=ON
# option(USE_LOCAL_VST3WF "Use local vst3wf sources instead of fetching" OFF)
# set(VST3WF_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/vst3wf" CACHE PATH "Path to local vst3wf (used when PROJECT1_USE_LOCAL_VST3WF=ON)")

# if(USE_LOCAL_VST3WF)
#     if(NOT EXISTS "${VST3WF_SOURCE_DIR}/CMakeLists.txt")
#         message(FATAL_ERROR "USE_LOCAL_VST3WF=ON but VST3WF_SOURCE_DIR is invalid: ${VST3WF_SOURCE_DIR}")
#     endif()
#     add_subdirectory("${VST3WF_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/vst3wf-local")
# else()
#     include(FetchContent)

#     set(VST3WF_GIT_REPOSITORY "https://github.com/yahiro07/sonic.git" CACHE STRING "vst3wf upstream repository")
#     set(VST3WF_GIT_TAG "main" CACHE STRING "vst3wf git tag/branch/commit")

#     FetchContent_Declare(
#         vst3wf
#         GIT_REPOSITORY ${VST3WF_GIT_REPOSITORY}
#         GIT_TAG        ${VST3WF_GIT_TAG}
#         GIT_SHALLOW    TRUE
#         SOURCE_SUBDIR  templates/vst-simple/libs/vst3wf
#     )
#     FetchContent_MakeAvailable(vst3wf)
# endif()

# add_subdirectory("sonic")

#----------------
#download sonic framework



#----------------

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
