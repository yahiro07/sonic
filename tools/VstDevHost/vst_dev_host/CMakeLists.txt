add_executable(VstDevHost 
    modules/midi_input_mac.cpp 
    modules/audio_io_mac.cpp
    main.cpp
    app0_mac_devices.cpp
    app1_plugin_interfacing.cpp
)

target_sources(VstDevHost PRIVATE modules/app_window_mac.mm)

target_link_libraries(VstDevHost PRIVATE
  sdk
  base
  pluginterfaces
  "-framework Cocoa"
  "-framework WebKit"
  "-framework CoreMIDI"
  "-framework CoreFoundation"
  "-framework AudioToolbox"
  "-framework CoreAudio"
)
