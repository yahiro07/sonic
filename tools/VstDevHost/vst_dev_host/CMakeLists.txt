set(SDK_ROOT "${SMTG_VST3_LOCATION_DIR}")

add_executable(VstDevHost)

target_sources(VstDevHost PRIVATE 
    modules/midi_input_mac.cpp 
    modules/audio_io_mac.cpp
    modules/app_window_mac.mm
    main.cpp
    app0_mac_devices.cpp
    app1_plugin_interfacing.cpp
    ${SDK_ROOT}/public.sdk/source/vst/hosting/plugprovider.cpp
    ${SDK_ROOT}/public.sdk/source/vst/hosting/module_mac.mm
)

set_source_files_properties(${SDK_ROOT}/public.sdk/source/vst/hosting/module_mac.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")

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
