#include "app_window_mac.h"
#include "midi_input_manager_mac.h"
#include <stdio.h>

int main() {
  printf("Hello VST Dev Host! 2026\n");
  // auto midiDevices = enumerateMidiInput();

  MidiInputManager midiIn;
  auto devices = midiIn.enumerateDevices();
  printf("Available MIDI Input Devices:\n");
  for (const auto &device : devices) {
    printf("[%s]: %s\n", device.deviceKey.c_str(), device.name.c_str());
  }
  if (devices.size() > 0) {
    printf("Opening MIDI Input Device: %s\n", devices[0].name.c_str());
    midiIn.open(devices[0].deviceKey,
                [](const std::vector<unsigned char> &message) {
                  printf("Received MIDI message: ");
                  for (unsigned char byte : message) {
                    printf("%02X ", byte);
                  }
                  printf("\n");
                });
  }
  printf("showing window...\n");
  AppWindowMac window;
  window.show();
  window.loop();

  printf("window closed, exiting...\n");
  midiIn.close();

  return 0;
}