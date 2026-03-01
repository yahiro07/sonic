add_executable(VstDevHost main.cpp 
    modules/midi_input_mac.cpp 
    modules/audio_io_mac.cpp
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
