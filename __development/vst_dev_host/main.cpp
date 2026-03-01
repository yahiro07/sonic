#include "midi_input.h"
#include "views.h"
#include <stdio.h>

int main() {
  printf("Hello VST Dev Host! 2026\n");
  // auto midiDevices = enumerateMidiInput();

  MidiInputManager midiIn;
  auto devices = MidiInputManager::enumerateDevices();
  printf("Available MIDI Input Devices:\n");
  for (const auto &device : devices) {
    printf("[%d]: %s\n", device.deviceId, device.name.c_str());
  }
  if (devices.size() > 0) {
    printf("Opening MIDI Input Device: %s\n", devices[0].name.c_str());
    midiIn.open(devices[0].deviceId,
                [](const std::vector<unsigned char> &message) {
                  printf("Received MIDI message: ");
                  for (unsigned char byte : message) {
                    printf("%02X ", byte);
                  }
                  printf("\n");
                });
  }
  printf("showing window...\n");
  showWindow();
  windowLoop();

  printf("window closed, exiting...\n");
  midiIn.close();

  return 0;
}