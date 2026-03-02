#include "./modules/app_window_mac.h"
#include "./modules/audio_io_mac.h"
#include "./modules/midi_input_mac.h"
#include "./modules/midi_packet_helper.h"
#include "./modules/spsc_queue.h"
#include "./vst/plugin_bridge.h"
#include "pluginterfaces/base/ftypes.h"
#include "pluginterfaces/base/funknown.h"
#include <filesystem>
#include <signal.h>
#include <stdio.h>

namespace vst_dev_host {

float randF() { return (float)rand() / (float)RAND_MAX; }

class Application {
  AppWindowMac window;
  MidiInputMac midiIn;
  AudioIoMac audioIo;
  PluginBridge pluginBridge;

  SPSCQueue<InputEvent, 256> inputEventQueue;
  InputEvent drainedEvents[256];
  // double sampleRate = 0.0;
  // float phase = 0.0f;

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

  void handleMidiShortMessage(const unsigned char *message, size_t length) {
    auto status = message[0];
    auto data1 = length >= 2 ? message[1] : 0;
    auto data2 = length >= 3 ? message[2] : 0;
    auto upper4 = status & 0xF0;
    if (upper4 == 0x90) {
      auto noteNumber = data1;
      auto velocity = (float)data2 / 127.0f;
      inputEventQueue.push(InputEvent{
          .type =
              velocity > 0 ? InputEventType::NoteOn : InputEventType::NoteOff,
          .id = static_cast<uint32_t>(noteNumber),
          .value = velocity,
      });
    } else if (upper4 == 0x80) {
      auto noteNumber = data1;
      inputEventQueue.push(InputEvent{
          .type = InputEventType::NoteOff,
          .id = static_cast<uint32_t>(noteNumber),
          .value = 0.0f,
      });
    } else if (upper4 == 0xE0) {
      int lsb = data1;
      int msb = data2;
      int value14 = (msb << 7) | lsb;                // 0〜16383
      float normalized = (value14 - 8192) / 8192.0f; // -1.0〜+1.0
      inputEventQueue.push(InputEvent{
          .type = InputEventType::PitchBend,
          .id = 0,
          .value = normalized,
      });
    }
  }

  void openAudioIo(const std::string &deviceKey) {
    audioIo.open(
        deviceKey, true,
        [this](double sr, int maxFrames) {
          this->pluginBridge.prepareAudio(sr, maxFrames);
        },
        [this](float *l, float *r, int n) {
          size_t eventCount = inputEventQueue.drain(drainedEvents, 256);
          this->pluginBridge.processAudio(l, r, n, drainedEvents, eventCount);
        });
  }

  void openMidiInput(const std::string &deviceKey) {
    midiIn.open(deviceKey, [this](const unsigned char *packet, size_t len) {
      decodeMidiPacketBytes(packet, len,
                            [this](const unsigned char *msg, size_t len) {
                              this->handleMidiShortMessage(msg, len);
                            });
    });
  }

public:
  void run() {
    printf("VstDevHost 0048\n");

    // Handle ctrl+c
    signal(SIGINT, [](int) {
      printf("\nCaught SIGINT, stopping...\n");
      // On macOS, we can stop the NSApp run loop
      extern void mac_stop_app();
      mac_stop_app();
    });

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

    pluginBridge.subscribeParameterEdit([this](uint32_t paramId, double value) {
      printf("Parameter edited: paramId=%d, value=%.02f\n", paramId, value);
      this->inputEventQueue.push(InputEvent{
          .type = InputEventType::ParameterChange,
          .id = paramId,
          .value = static_cast<float>(value),
      });
    });

    window.loop();

    printf("window closed, exiting...\n");
    pluginBridge.unsubscribeParameterEdit();
    midiIn.close();
    audioIo.close();
    window.unsubscribeMidiInputDeviceSelection();
    window.unsubscribeAudioDeviceSelection();

    pluginBridge.closeEditor();
    pluginBridge.unloadPlugin();
    printf("cleanup done\n");
  }
};

} // namespace vst_dev_host

void app0Entry() {
  Steinberg::tresult res = Steinberg::kResultOk;
  printf("r: %d\n", res);
  vst_dev_host::Application app;
  app.run();
}