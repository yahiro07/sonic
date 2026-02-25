cmake_minimum_required(VERSION 3.14.0)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15 CACHE STRING "")

project(Project1
    VERSION 1.0.0.0
    DESCRIPTION "Project1 VST 3 Plug-in"
)

add_subdirectory(external)

smtg_enable_vst3_sdk()

smtg_add_vst3plugin(Project1
    source/version.h
    source/project1_cids.h
    source/project1_processor.h
    source/project1_processor.cpp
    source/project1_controller.h
    source/project1_controller.cpp
    source/project1_entry.cpp
    source/dsp/MySynthesizer.cpp
    source/general/logger.cpp
    source/logic/parameter_builder_impl.cpp
    source/wrapper/edit_controller_parameter_change_notifier.cpp
    source/wrapper/parameters_manager.cpp
    source/wrapper/webview_editor_view.cpp

)

target_link_libraries(Project1
    PRIVATE
        sdk
)

smtg_target_configure_version_file(Project1)




add_subdirectory(libs/vst3wf)

target_link_libraries(Project1
    PRIVATE
        vst3wf
)
target_include_directories(Project1
    PRIVATE
        libs/vst3wf
)




# Keep web resources in sync with the built plug-in bundle.
# This registers every file under resource/www (recursively) as a plug-in resource
# and preserves the folder layout under <Bundle>/Contents/Resources/www/...
set(PROJECT1_WWW_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/resource/www")
file(GLOB_RECURSE PROJECT1_WWW_FILES CONFIGURE_DEPENDS "${PROJECT1_WWW_ROOT}/*")
foreach(_www_file IN LISTS PROJECT1_WWW_FILES)
    if(IS_DIRECTORY "${_www_file}")
        continue()
    endif()

    file(RELATIVE_PATH _www_relpath "${PROJECT1_WWW_ROOT}" "${_www_file}")
    get_filename_component(_www_reldir "${_www_relpath}" DIRECTORY)

    if(_www_reldir STREQUAL "")
        set(_www_outsubdir "www")
    else()
        set(_www_outsubdir "www/${_www_reldir}")
    endif()

    smtg_target_add_plugin_resource(Project1 "${_www_file}" "${_www_outsubdir}")
endforeach()

FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG main
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glaze)
target_link_libraries(Project1 PRIVATE glaze::glaze)



if(SMTG_MAC)
    smtg_target_set_bundle(Project1
        BUNDLE_IDENTIFIER com.mycompany.project1
        COMPANY_NAME "MyCompany"
    )
    smtg_target_set_debug_executable(Project1
        "/Applications/VST3PluginTestHost.app"
        "--pluginfolder;$(BUILT_PRODUCTS_DIR)"
    )

    target_sources(Project1 PRIVATE
        source/general/mac_web_view.mm
    )

    target_link_libraries(Project1 PRIVATE
        "-framework Cocoa"
        "-framework Webkit"
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
