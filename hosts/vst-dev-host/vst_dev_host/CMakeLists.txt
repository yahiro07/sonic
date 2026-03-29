add_executable(VstDevHost)

target_sources(
  VstDevHost
  PRIVATE modules/midi_input_mac.cpp
          modules/audio_io_mac.cpp
          modules/app_window_mac.mm
          modules/logger.cpp
          vst/plugin_bridge.cpp
          main.cpp
          ${VST3_SDK_ROOT}/public.sdk/source/vst/hosting/plugprovider.cpp
          ${VST3_SDK_ROOT}/public.sdk/source/vst/hosting/module_mac.mm)

set_source_files_properties(
  ${VST3_SDK_ROOT}/public.sdk/source/vst/hosting/module_mac.mm
  PROPERTIES COMPILE_FLAGS "-fobjc-arc")

target_link_libraries(
  VstDevHost
  PRIVATE sdk
          sdk_hosting
          "-framework Cocoa"
          "-framework WebKit"
          "-framework CoreMIDI"
          "-framework CoreFoundation"
          "-framework AudioToolbox"
          "-framework CoreAudio")
