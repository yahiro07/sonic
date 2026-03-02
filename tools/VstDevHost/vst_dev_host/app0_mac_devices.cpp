#include "./modules/app_window_mac.h"
#include "./modules/audio_io_mac.h"
#include "./modules/midi_input_mac.h"
#include "./vst/plugin_bridge.h"
#include "pluginterfaces/base/ftypes.h"
#include "pluginterfaces/base/funknown.h"
#include <filesystem>
#include <stdio.h>

namespace vst_dev_host {

float randF() { return (float)rand() / (float)RAND_MAX; }

class Application {
  AppWindowMac window;
  MidiInputMac midiIn;
  AudioIoMac audioIo;
  PluginBridge pluginBridge;

  double sampleRate = 0.0;
  float phase = 0.0f;

  // void audioPrepareFn(double sampleRate, int maxFrameLength) {
  //   printf("sampleRate:%.0f, bufferSize:%d\n", sampleRate, maxFrameLength);
  //   this->sampleRate = sampleRate;
  // }

  // void audioProcessFn(float *bufferL, float *bufferR, int nframes) {
  //   if (sampleRate == 0.0)
  //     return;
  //   float delta = 440.0f / (float)sampleRate;
  //   for (int i = 0; i < nframes; i++) {
  //     phase += delta;
  //     if (phase > 1.0)
  //       phase -= 1.0;
  //     // auto y = sin(phase * 2.0 * M_PI);
  //     float y = (randF() * 2.0f - 1.0f) * 0.2f;
  //     bufferL[i] = y;
  //     bufferR[i] = y;
  //   }
  // }

  void handleMidiMessage(const std::vector<unsigned char> &message) {
    printf("Received MIDI message: ");
    for (unsigned char byte : message) {
      printf("%02X ", byte);
    }
    printf("\n");
  }

  void openAudioIo(const std::string &deviceKey) {
    audioIo.open(
        deviceKey, true,
        [this](double sr, int maxFrames) {
          this->pluginBridge.prepareAudio(sr, maxFrames);
        },
        [this](float *l, float *r, int n) {
          this->pluginBridge.processAudio(l, r, n);
        });
  }

  void openMidiInput(const std::string &deviceKey) {
    midiIn.open(deviceKey, [this](const std::vector<unsigned char> &message) {
      this->handleMidiMessage(message);
    });
  }

public:
  void run() {
    printf("VstDevHost 0048\n");

    std::filesystem::path relPath =
        "../../templates/vst-simple/build/VST3/Debug/Project1.vst3";
    auto vstPath =
        std::filesystem::absolute(relPath).lexically_normal().string();

    pluginBridge.loadPlugin(vstPath);
    window.show();
    pluginBridge.createEditor(window.getViewHandle());

    auto audioDevices = audioIo.enumerateDevices();
    auto midiDevices = midiIn.enumerateDevices();

    if (audioDevices.size() > 0) {
      auto initialDeviceKey = audioDevices[0].deviceKey;
      printf("Opening Audio Device: %s\n", audioDevices[0].displayName.c_str());
      openAudioIo(initialDeviceKey);
      window.refreshAudioDeviceListMenu(audioDevices, initialDeviceKey);
    }
    if (midiDevices.size() > 0) {
      auto initialDeviceKey = midiDevices[0].deviceKey;
      printf("Opening MIDI Input Device: %s\n",
             midiDevices[0].displayName.c_str());
      openMidiInput(initialDeviceKey);
      window.refreshMidiInputDeviceListMenu(midiDevices, initialDeviceKey);
    }

    window.subscribeAudioDeviceSelection(
        [this, audioDevices](const std::string &deviceKey) {
          this->audioIo.close();
          this->openAudioIo(deviceKey);
          window.refreshAudioDeviceListMenu(audioDevices, deviceKey);
        });
    window.subscribeMidiInputDeviceSelection(
        [this, midiDevices](const std::string &deviceKey) {
          this->midiIn.close();
          this->openMidiInput(deviceKey);
          window.refreshMidiInputDeviceListMenu(midiDevices, deviceKey);
        });

    window.loop();

    printf("window closed, exiting...\n");
    midiIn.close();
    audioIo.close();
    window.unsubscribeMidiInputDeviceSelection();
    window.unsubscribeAudioDeviceSelection();

    pluginBridge.closeEditor();
    pluginBridge.unloadPlugin();
  }
};

} // namespace vst_dev_host

void app0Entry() {
  Steinberg::tresult res = Steinberg::kResultOk;
  printf("r: %d\n", res);
  vst_dev_host::Application app;
  app.run();
}