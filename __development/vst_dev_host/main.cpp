#include "app_window_mac.h"
#include "audio_io_mac.h"
#include "midi_input_mac.h"
#include <stdio.h>

void handleMidiMessage(const std::vector<unsigned char> &message) {
  printf("Received MIDI message: ");
  for (unsigned char byte : message) {
    printf("%02X ", byte);
  }
  printf("\n");
}
AppWindowMac window;
MidiInputMac midiIn;
AudioIoMac audioIo;

double gSampleRate = 0.0;
float phase = 0.0f;

void audioPrepareFn(double sampleRate, int maxFrameLength) {
  printf("Audio Prepare: sampleRate=%f, maxFrameLength=%d\n", sampleRate,
         maxFrameLength);
  gSampleRate = sampleRate;
}

float randF() { return (float)rand() / (float)RAND_MAX; }

void audioProcessFn(float *bufferL, float *bufferR, int nframes) {
  if (gSampleRate == 0.0)
    return;
  float delta = 440.0f / (float)gSampleRate;
  for (int i = 0; i < nframes; i++) {
    phase += delta;
    if (phase > 1.0)
      phase -= 1.0;
    // auto y = sin(phase * 2.0 * M_PI);
    float y = (randF() * 2.0f - 1.0f) * 0.2f;
    bufferL[i] = y;
    bufferR[i] = y;
  }
}

int main() {
  printf("Hello VST Dev Host! 2224\n");
  auto devices = midiIn.enumerateDevices();
  printf("Available MIDI Input Devices:\n");
  for (const auto &device : devices) {
    printf("[%s]: %s\n", device.deviceKey.c_str(), device.displayName.c_str());
  }
  if (devices.size() > 0) {
    printf("Opening MIDI Input Device: %s\n", devices[0].displayName.c_str());
    midiIn.open(devices[0].deviceKey, handleMidiMessage);
  }
  printf("showing window...\n");
  window.show();
  {
    auto initialDeviceKey = devices.size() > 0 ? devices[0].deviceKey : "";
    window.refreshMidiInputDeviceListMenu(devices, initialDeviceKey);
    window.subscribeMidiInputDeviceSelection([](const std::string &deviceKey) {
      midiIn.open(deviceKey, handleMidiMessage);
    });
  }

  auto audioDevices = audioIo.enumerateDevices();
  printf("Available Audio Output Devices:\n");
  for (const auto &device : audioDevices) {
    printf("[%s]: %s\n", device.deviceKey.c_str(), device.displayName.c_str());
  }
  audioIo.open(audioDevices[0].deviceKey, true, audioPrepareFn, audioProcessFn);

  window.loop();

  printf("window closed, exiting...\n");
  midiIn.close();
  audioIo.close();
  window.clearMenuSubscriptions();

  return 0;
}