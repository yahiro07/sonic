#include "app_window_mac.h"
#include "audio_io_mac.h"
#include "midi_input_mac.h"
#include <stdio.h>

float randF() { return (float)rand() / (float)RAND_MAX; }

class Application {
  AppWindowMac window;
  MidiInputMac midiIn;
  AudioIoMac audioIo;

  double sampleRate = 0.0;
  float phase = 0.0f;

  void audioPrepareFn(double sampleRate, int maxFrameLength) {
    printf("Audio Prepare: sampleRate=%f, maxFrameLength=%d\n", sampleRate,
           maxFrameLength);
    this->sampleRate = sampleRate;
  }

  void audioProcessFn(float *bufferL, float *bufferR, int nframes) {
    if (sampleRate == 0.0)
      return;
    float delta = 440.0f / (float)sampleRate;
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

  void handleMidiMessage(const std::vector<unsigned char> &message) {
    printf("Received MIDI message: ");
    for (unsigned char byte : message) {
      printf("%02X ", byte);
    }
    printf("\n");
  }

public:
  void run() {
    printf("Hello VST Dev Host! 2224\n");
    auto audioDevices = audioIo.enumerateDevices();
    auto midiDevices = midiIn.enumerateDevices();
    printf("Available Audio Output Devices:\n");
    for (const auto &device : audioDevices) {
      printf("[%s]: %s\n", device.deviceKey.c_str(),
             device.displayName.c_str());
    }
    printf("Available MIDI Input Devices:\n");
    for (const auto &device : midiDevices) {
      printf("[%s]: %s\n", device.deviceKey.c_str(),
             device.displayName.c_str());
    }

    auto initialAudioDeviceKey = audioDevices[0].deviceKey;
    auto initialMidiDeviceKey =
        midiDevices.size() > 0 ? midiDevices[0].deviceKey : "";

    audioIo.open(
        initialAudioDeviceKey, true,
        [this](double sr, int maxFrames) {
          this->audioPrepareFn(sr, maxFrames);
        },
        [this](float *l, float *r, int n) { this->audioProcessFn(l, r, n); });
    if (initialMidiDeviceKey != "") {
      printf("Opening MIDI Input Device: %s\n",
             midiDevices[0].displayName.c_str());
      midiIn.open(initialMidiDeviceKey,
                  [this](const std::vector<unsigned char> &message) {
                    this->handleMidiMessage(message);
                  });
    }
    printf("showing window...\n");
    window.show();

    window.refreshAudioDeviceListMenu(audioDevices, initialAudioDeviceKey);
    window.refreshMidiInputDeviceListMenu(midiDevices, initialMidiDeviceKey);

    window.subscribeAudioDeviceSelection([this](const std::string &deviceKey) {
      this->audioIo.close();
      this->audioIo.open(
          deviceKey, true,
          [this](double sr, int maxFrames) {
            this->audioPrepareFn(sr, maxFrames);
          },
          [this](float *l, float *r, int n) { this->audioProcessFn(l, r, n); });
    });
    window.subscribeMidiInputDeviceSelection(
        [this](const std::string &deviceKey) {
          this->midiIn.close();
          this->midiIn.open(deviceKey,
                            [this](const std::vector<unsigned char> &message) {
                              this->handleMidiMessage(message);
                            });
        });

    window.loop();

    printf("window closed, exiting...\n");
    midiIn.close();
    audioIo.close();
    window.unsubscribeMidiInputDeviceSelection();
    window.unsubscribeAudioDeviceSelection();
  }
};

int main() {
  Application app;
  app.run();
  return 0;
}