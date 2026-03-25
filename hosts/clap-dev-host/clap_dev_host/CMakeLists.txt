set(NAME clap_dev_host)

add_executable(${NAME})

target_sources(
  ${NAME} PRIVATE main.cpp modules/app_window_mac.mm modules/audio_io_mac.cpp
                  modules/midi_input_mac.cpp plugin_bridge/plugin_bridge.cpp)

target_link_libraries(
  ${NAME}
  PRIVATE clap
          dl
          "-framework CoreAudio"
          "-framework AudioToolbox"
          "-framework CoreMIDI"
          "-framework Cocoa"
          "-framework QuartzCore")
