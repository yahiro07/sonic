set(HOST_SOURCES
    main.cpp
    modules/app_window_mac.mm
    modules/audio_io_mac.cpp
    modules/midi_input_mac.cpp
    plugin_bridge/plugin_bridge.cpp
)

add_executable(clap_dev_host ${HOST_SOURCES})

target_include_directories(clap_dev_host PRIVATE 
    .
    ./modules
    ./plugin_bridge
    ${clap_SOURCE_DIR}/include
)

if(APPLE)
    target_link_libraries(clap_dev_host PRIVATE 
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework CoreMIDI"
        "-framework Cocoa"
        "-framework QuartzCore"
        dl
    )
elseif(UNIX)
    target_link_libraries(clap_dev_host PRIVATE dl pthread)
endif()
