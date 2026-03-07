#pragma once
#include <functional>
#include <memory>
#include <string>

namespace clap_dev_host {

class HostPlugFrame;

enum class InputEventType : uint8_t {
  NoteOn,
  NoteOff,
  ParameterChange,
};

struct InputEvent {
  InputEventType type;
  uint8_t __padding[3];
  //[id, value] represents
  // [noteNumber, velocity] for NoteOn/NoteOff
  // [paramId, paramValue] for ParameterChange
  uint32_t id;
  float value;
};

class PluginBridge {
public:
  PluginBridge();
  ~PluginBridge();

  bool loadPlugin(const std::string &path);
  void getDesiredEditorSize(int &width, int &height);
  void openEditor(void *ownerViewHandle);
  void closeEditor();
  bool requestEditorResize(int &width, int &height);
  void subscribeEditorSizeChangeRequest(
      std::function<bool(int width, int height)> callback);
  void unsubscribeEditorSizeChangeRequest();

  void unloadPlugin();
  void prepareAudio(double sampleRate, int maxBlockSize);
  void processAudio(float *bufferL, float *bufferR, int nframes,
                    const InputEvent *events, size_t eventCount);

  void subscribeParameterEdit(std::function<void(uint32_t, double)> fn);
  void unsubscribeParameterEdit();

private:
  struct PluginBridgeInternal;
  std::unique_ptr<PluginBridgeInternal> states;
};

} // namespace clap_dev_host