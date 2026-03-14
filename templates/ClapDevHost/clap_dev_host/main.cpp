#include "./modules/app_window_mac.h"
#include "./modules/audio_io_mac.h"
#include "./modules/midi_input_mac.h"
#include "./modules/midi_packet_helper.h"
#include "./modules/spsc_queue.h"
#include "./plugin_bridge/plugin_bridge.h"
#include <filesystem>
#include <signal.h>
#include <stdio.h>
#include <string>

namespace clap_dev_host {

float randF() { return (float)rand() / (float)RAND_MAX; }

class Application {
  AppWindowMac window;
  MidiInputMac midiIn;
  AudioIoMac audioIo;
  PluginBridge pluginBridge;

  SPSCQueue<InputEvent, 256> inputEventQueue;
  InputEvent drainedEvents[256];

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
    printf("ClapDevHost\n");

    // Handle ctrl+c
    signal(SIGINT, [](int) {
      printf("\nCaught SIGINT, stopping...\n");
      // On macOS, we can stop the NSApp run loop
      extern void mac_stop_app();
      mac_stop_app();
    });

    std::filesystem::path path(pluginPath);

    // If the path ends with .clap and it's a directory (bundle),
    // point to the executable inside the bundle.
    if (path.extension() == ".clap" && std::filesystem::is_directory(path)) {
      auto name = path.stem().string();
      auto binaryPath = path / "Contents" / "MacOS" / name;
      if (std::filesystem::exists(binaryPath)) {
        path = binaryPath;
      }
    }

    auto vstPath = std::filesystem::absolute(path).lexically_normal().string();
    printf("Loading plugin: %s\n", vstPath.c_str());

    if (!std::filesystem::exists(vstPath)) {
      printf("Error: Plugin file does not exist: %s\n", vstPath.c_str());
      return;
    }

    auto loaded = pluginBridge.loadPlugin(vstPath);
    if (!loaded) {
      printf("Failed to load plugin: %s\n", vstPath.c_str());
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
    pluginBridge.unsubscribeEditorSizeChangeRequest();
    midiIn.close();
    audioIo.close();
    window.unsubscribeWindowResize();
    window.unsubscribeMidiInputDeviceSelection();
    window.unsubscribeAudioDeviceSelection();

    pluginBridge.closeEditor();
    pluginBridge.unloadPlugin();
    printf("cleanup done\n");
  }
};

} // namespace clap_dev_host

int main(int argc, char **argv) {
  clap_dev_host::Application app;

  if (argc >= 2 && argv && argv[1]) {
    std::string arg1 = argv[1];
    if (arg1 == "--help" || arg1 == "-h") {
      printf("Usage: ClapDevHost path/to/plugin.clap [--smoke]\n");
    } else if (arg1 == "--version" || arg1 == "-v") {
      printf("ClapDevHost version 0.0.1\n");
    } else {
      auto &pluginPath = arg1;
      const bool smoke =
          (argc >= 3 && argv[2] && std::string(argv[2]) == "--smoke");
      app.run(pluginPath, smoke);
    }
  } else {
    printf("Usage: ClapDevHost path/to/plugin.clap [--smoke]\n");
  }
  return 0;
}