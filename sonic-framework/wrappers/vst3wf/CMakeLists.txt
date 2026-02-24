cmake_minimum_required(VERSION 3.14.0)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13 CACHE STRING "")

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
    source/logger.cpp
)

target_link_libraries(Project1
    PRIVATE
        sdk
)

smtg_target_add_plugin_resources(Project1
    RESOURCES
        resource/www
)

smtg_target_configure_version_file(Project1)

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
      source/webview/mac_web_view.mm
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
endif(SMTG_MAC)
