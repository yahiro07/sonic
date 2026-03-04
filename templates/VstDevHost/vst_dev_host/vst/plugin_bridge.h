#pragma once
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "public.sdk/source/vst/hosting/eventlist.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include <functional>
#include <string>

namespace vst_dev_host {

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
  void createEditor(void *ownerViewHandle);
  void closeEditor();
  void unloadPlugin();
  void prepareAudio(double sampleRate, int maxBlockSize);
  void processAudio(float *bufferL, float *bufferR, int nframes,
                    const InputEvent *events, size_t eventCount);

  void subscribeParameterEdit(std::function<void(uint32_t, double)> fn);
  void unsubscribeParameterEdit();

private:
  Steinberg::Vst::HostApplication hostApp;
  VST3::Hosting::Module::Ptr module;
  Steinberg::IPtr<Steinberg::Vst::IComponent> component;
  Steinberg::FUnknownPtr<Steinberg::Vst::IAudioProcessor> audioProcessor;
  Steinberg::FUnknownPtr<Steinberg::Vst::IEditController> editController;
  Steinberg::IPlugView *plugView = nullptr;
  bool controllerIsComponent = false;
  bool isConnected = false;
  bool isActive = false;
  bool isProcessing = false;

  Steinberg::Vst::EventList eventList;
  Steinberg::Vst::ParameterChanges paramChanges;
  class ComponentHandler;
  Steinberg::IPtr<ComponentHandler> componentHandler = nullptr;
};

} // namespace vst_dev_host