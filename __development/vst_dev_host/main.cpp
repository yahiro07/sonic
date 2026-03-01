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
  auto audioDevices = audioIo.enumerateDevices();
  auto midiDevices = midiIn.enumerateDevices();
  printf("Available Audio Output Devices:\n");
  for (const auto &device : audioDevices) {
    printf("[%s]: %s\n", device.deviceKey.c_str(), device.displayName.c_str());
  }
  printf("Available MIDI Input Devices:\n");
  for (const auto &device : midiDevices) {
    printf("[%s]: %s\n", device.deviceKey.c_str(), device.displayName.c_str());
  }

  auto initialAudioDeviceKey = audioDevices[0].deviceKey;
  auto initialMidiDeviceKey =
      midiDevices.size() > 0 ? midiDevices[0].deviceKey : "";

  audioIo.open(initialAudioDeviceKey, true, audioPrepareFn, audioProcessFn);
  if (initialMidiDeviceKey != "") {
    printf("Opening MIDI Input Device: %s\n",
           midiDevices[0].displayName.c_str());
    midiIn.open(initialMidiDeviceKey, handleMidiMessage);
  }
  printf("showing window...\n");
  window.show();

  window.refreshAudioDeviceListMenu(audioDevices, initialAudioDeviceKey);
  window.refreshMidiInputDeviceListMenu(midiDevices, initialMidiDeviceKey);

  window.subscribeAudioDeviceSelection([](const std::string &deviceKey) {
    audioIo.close();
    audioIo.open(deviceKey, true, audioPrepareFn, audioProcessFn);
  });
  window.subscribeMidiInputDeviceSelection([](const std::string &deviceKey) {
    midiIn.open(deviceKey, handleMidiMessage);
  });

  window.loop();

  printf("window closed, exiting...\n");
  midiIn.close();
  audioIo.close();
  window.unsubscribeMidiInputDeviceSelection();
  window.unsubscribeAudioDeviceSelection();

  return 0;
}