#include "app_window_mac.h"
#include "midi_input_manager_mac.h"
#include <stdio.h>

void handleMidiMessage(const std::vector<unsigned char> &message) {
  printf("Received MIDI message: ");
  for (unsigned char byte : message) {
    printf("%02X ", byte);
  }
  printf("\n");
}
AppWindowMac window;
MidiInputManager midiIn;

int main() {
  printf("Hello VST Dev Host! 2224\n");
  auto devices = midiIn.enumerateDevices();
  printf("Available MIDI Input Devices:\n");
  for (const auto &device : devices) {
    printf("[%s]: %s\n", device.deviceKey.c_str(), device.name.c_str());
  }
  if (devices.size() > 0) {
    printf("Opening MIDI Input Device: %s\n", devices[0].name.c_str());
    midiIn.open(devices[0].deviceKey, handleMidiMessage);
  }
  printf("showing window...\n");
  window.show();
  auto initialDeviceKey = devices.size() > 0 ? devices[0].deviceKey : "";
  window.refreshMidiInputDeviceListMenu(devices, initialDeviceKey);
  window.subscribeMidiInputDeviceSelection([](const std::string &deviceKey) {
    midiIn.open(deviceKey, handleMidiMessage);
  });
  window.loop();

  printf("window closed, exiting...\n");
  midiIn.close();
  window.clearMenuSubscriptions();

  return 0;
}