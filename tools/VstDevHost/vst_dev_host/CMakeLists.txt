add_executable(VstDevHost 
    modules/midi_input_mac.cpp 
    modules/audio_io_mac.cpp
    main.cpp
    app0_mac_devices.cpp
    app1_plugin_interfacing.cpp
)

target_sources(VstDevHost PRIVATE 
    modules/app_window_mac.mm
    ${SMTG_VST3_LOCATION_DIR}/public.sdk/source/vst/hosting/module_mac.mm
)

set_source_files_properties(${SMTG_VST3_LOCATION_DIR}/public.sdk/source/vst/hosting/module_mac.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")

target_link_libraries(VstDevHost PRIVATE
  sdk
  sdk_hosting
  base
  pluginterfaces
  "-framework Cocoa"
  "-framework WebKit"
  "-framework CoreMIDI"
  "-framework CoreFoundation"
  "-framework AudioToolbox"
  "-framework CoreAudio"
)
