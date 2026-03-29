#include "./modules/app_window_mac.h"
#include "./modules/audio_io_mac.h"
#include "./modules/logger.h"
#include "./modules/midi_input_mac.h"
#include "./modules/midi_packet_helper.h"
#include "./modules/spsc_queue.h"
#include "./vst/plugin_bridge.h"
#include "pluginterfaces/base/ftypes.h"
#include "pluginterfaces/base/funknown.h"
#include <chrono>
#include <filesystem>
#include <signal.h>
#include <stdio.h>
#include <string>
#include <thread>
#include <unistd.h>

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
  void run(const std::string &pluginPath, bool smoke = false) {
    logger.start();
    logger.trace("VstDevHost");

    // Handle ctrl+c
    signal(SIGINT, [](int) {
      printf("\n");
      logger.log("Caught SIGINT, stopping...");
      // On macOS, we can stop the NSApp run loop
      extern void mac_stop_app();
      mac_stop_app();
    });

    std::filesystem::path path(pluginPath);
    auto vstPath = std::filesystem::absolute(path).lexically_normal().string();
    logger.log("Loading plugin: %s", vstPath.c_str());

    auto loaded = pluginBridge.loadPlugin(vstPath);
    if (!loaded) {
      logger.error("Failed to load plugin: %s", vstPath.c_str());
      return;
    }

    if (smoke) {
      printf("Smoke mode: unloading immediately\n");
      pluginBridge.unloadPlugin();
      printf("Smoke mode: done\n");
      return;
    }

    window.show();
    int editorWidth = 0, editorHeight = 0;
    pluginBridge.getDesiredEditorSize(editorWidth, editorHeight);
    if (editorWidth > 0 && editorHeight > 0) {
      window.setWindowSize(editorWidth, editorHeight);
    }
    pluginBridge.openEditor(window.getViewHandle());
    window.subscribeWindowResize([this](int width, int height) {
      int w = width;
      int h = height;
      (void)pluginBridge.requestEditorResize(w, h);
      window.setWindowSize(w, h);
    });
    pluginBridge.subscribeEditorSizeChangeRequest(
        [this](int width, int height) -> bool {
          window.setWindowSize(width, height);
          return true;
        });

    auto audioDevices = audioIo.enumerateDevices();
    auto midiDevices = midiIn.enumerateDevices();

    if (audioDevices.size() > 0) {
      auto initialDeviceKey = audioDevices[0].deviceKey;
      logger.log("Opening Audio Device: %s",
                 audioDevices[0].displayName.c_str());
      openAudioIo(initialDeviceKey);
      window.refreshAudioDeviceListMenu(audioDevices, initialDeviceKey);
    }
    if (midiDevices.size() > 0) {
      auto initialDeviceKey = midiDevices[0].deviceKey;
      logger.log("Opening MIDI Input Device: %s",
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
      // logger.log("Parameter edited: paramId=%d, value=%.02f\n", paramId,
      // value);
      this->inputEventQueue.push(InputEvent{
          .type = InputEventType::ParameterChange,
          .id = paramId,
          .value = static_cast<float>(value),
      });
    });

    window.loop();

    logger.log("window closed, exiting...");
    pluginBridge.unsubscribeParameterEdit();
    pluginBridge.unsubscribeEditorSizeChangeRequest();
    midiIn.close();
    audioIo.close();
    window.unsubscribeWindowResize();
    window.unsubscribeMidiInputDeviceSelection();
    window.unsubscribeAudioDeviceSelection();

    pluginBridge.closeEditor();
    pluginBridge.unloadPlugin();
    logger.log("cleanup done");
    // sleep a bit to wait logs to be sent
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    logger.stop();
  }
};

} // namespace vst_dev_host

int main(int argc, char **argv) {
  vst_dev_host::Application app;

  if (argc >= 2 && argv && argv[1]) {
    std::string arg1 = argv[1];
    if (arg1 == "--help" || arg1 == "-h") {
      printf("Usage: VstDevHost path/to/plugin.vst3 [--smoke]\n");
    } else if (arg1 == "--version" || arg1 == "-v") {
      printf("VstDevHost version 0.0.1\n");
    } else {
      auto &pluginPath = arg1;
      const bool smoke =
          (argc >= 3 && argv[2] && std::string(argv[2]) == "--smoke");
      app.run(pluginPath, smoke);
    }
  } else {
    printf("Usage: VstDevHost path/to/plugin.vst3 [--smoke]\n");
  }
  return 0;
}