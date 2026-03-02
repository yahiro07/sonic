#include "plugin_bridge.h"
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include <cstdio>

namespace vst_dev_host {

using namespace Steinberg;
using namespace Steinberg::Vst;

class PluginBridge::ComponentHandler
    : public Steinberg::Vst::IComponentHandler {
public:
  ComponentHandler() {}
  ~ComponentHandler() {}

  tresult PLUGIN_API beginEdit(Vst::ParamID paramId) override {
    return kResultTrue;
  }

  tresult PLUGIN_API performEdit(Vst::ParamID paramId, double value) override {
    if (parameterEditCallback) {
      parameterEditCallback(paramId, value);
    }
    return kResultTrue;
  }

  tresult PLUGIN_API endEdit(Vst::ParamID paramId) override {
    return kResultTrue;
  }

  tresult PLUGIN_API restartComponent(int32_t flags) override {
    return kResultTrue;
  }

  void setParameterEditCallback(
      std::function<void(uint32_t paramId, double value)> fn) {
    parameterEditCallback = fn;
  }
  void clearParameterEditCallback() { parameterEditCallback = nullptr; }

  DECLARE_FUNKNOWN_METHODS
private:
  std::function<void(uint32_t paramId, double value)> parameterEditCallback =
      nullptr;
};
IMPLEMENT_FUNKNOWN_METHODS(PluginBridge::ComponentHandler, IComponentHandler,
                           IComponentHandler::iid)

PluginBridge::PluginBridge() {}

PluginBridge::~PluginBridge() { unloadPlugin(); }

void PluginBridge::loadPlugin(const std::string &path) {
  printf("PluginBridge::loadPlugin: %s\n", path.c_str());

  std::string errorDescription;
  module = VST3::Hosting::Module::create(path, errorDescription);
  if (!module) {
    printf("Failed to load module: %s\n", errorDescription.c_str());
    return;
  }

  auto factory = module->getFactory();
  VST3::Hosting::ClassInfo audioEffectClassInfo;
  bool found = false;
  for (auto &classInfo : factory.classInfos()) {
    if (classInfo.category() == kVstAudioEffectClass) {
      audioEffectClassInfo = classInfo;
      found = true;
      break;
    }
  }

  if (!found) {
    printf("No audio effect class found in plugin\n");
    return;
  }

  plugProvider = new PlugProvider(factory, audioEffectClassInfo, true);
  if (!plugProvider) {
    printf("Failed to create PlugProvider\n");
    return;
  }

  auto component = plugProvider->getComponent();
  if (!component) {
    printf("Failed to get component from PlugProvider\n");
    return;
  }

  audioProcessor = FUnknownPtr<IAudioProcessor>(component);
  if (!audioProcessor) {
    printf("Component does not support IAudioProcessor\n");
  }

  editController = plugProvider->getController();
  if (!editController) {
    printf("Failed to get edit controller\n");
  }

  componentHandler = IPtr<ComponentHandler>(new ComponentHandler(), false);
  editController->setComponentHandler(componentHandler);
}

void PluginBridge::createEditor(void *ownerViewHandle) {
  if (!editController)
    return;

  if (plugView) {
    closeEditor();
  }

  plugView = editController->createView(ViewType::kEditor);
  if (!plugView) {
    printf("Failed to create IPlugView\n");
    return;
  }

  ViewRect rect;
  if (plugView->getSize(&rect) != kResultTrue) {
    printf("Failed to get view size\n");
  }

  if (plugView->attached(ownerViewHandle, kPlatformTypeNSView) != kResultTrue) {
    printf("Failed to attach view to owner\n");
    plugView->release();
    plugView = nullptr;
  }
}

void PluginBridge::closeEditor() {
  if (plugView) {
    plugView->removed();
    plugView->release();
    plugView = nullptr;
  }
}

void PluginBridge::unloadPlugin() {
  if (editController) {
    editController->setComponentHandler(nullptr);
  }
  closeEditor();
  audioProcessor = nullptr;
  editController = nullptr;
  plugProvider = nullptr;
  module = nullptr;
  componentHandler = nullptr;
}

void PluginBridge::prepareAudio(double sampleRate, int maxBlockSize) {
  if (audioProcessor) {
    ProcessSetup setup;
    setup.processMode = kRealtime;
    setup.symbolicSampleSize = kSample32;
    setup.maxSamplesPerBlock = maxBlockSize;
    setup.sampleRate = sampleRate;

    audioProcessor->setupProcessing(setup);
    audioProcessor->setProcessing(true);

    eventList.setMaxSize(128);
    paramChanges.setMaxParameters(128);
  }
}

static void loadProcessDataFromInputEvents(ProcessData &data,
                                           Vst::EventList &eventList,
                                           Vst::ParameterChanges &paramChanges,
                                           const InputEvent *events,
                                           size_t eventCount) {
  eventList.clear();
  paramChanges.clearQueue();

  for (size_t i = 0; i < eventCount; ++i) {
    const auto &e = events[i];

    if (e.type == InputEventType::NoteOn || e.type == InputEventType::NoteOff) {
      Vst::Event vstEvent{};
      vstEvent.busIndex = 0;
      vstEvent.sampleOffset = 0;
      vstEvent.ppqPosition = 0;

      if (e.type == InputEventType::NoteOn) {
        vstEvent.type = Vst::Event::kNoteOnEvent;
        vstEvent.noteOn.channel = 0;
        vstEvent.noteOn.pitch = static_cast<int16>(e.id);
        vstEvent.noteOn.velocity = e.value;
      } else {
        vstEvent.type = Vst::Event::kNoteOffEvent;
        vstEvent.noteOff.channel = 0;
        vstEvent.noteOff.pitch = static_cast<int16>(e.id);
        vstEvent.noteOff.velocity = e.value;
      }

      eventList.addEvent(vstEvent);
    } else if (e.type == InputEventType::ParameterChange) {
      int32 index = 0;
      auto *queue = paramChanges.addParameterData(e.id, index);
      if (queue) {
        queue->addPoint(0, e.value, index);
      }
    }
  }

  data.inputEvents = &eventList;
  data.inputParameterChanges = &paramChanges;
}

void PluginBridge::processAudio(float *bufferL, float *bufferR, int nframes,
                                const InputEvent *events, size_t eventCount) {
  if (!audioProcessor) {
    return;
  }

  ProcessData data;
  data.processMode = kRealtime;
  data.symbolicSampleSize = kSample32;
  data.numSamples = nframes;

  AudioBusBuffers inputs_buf;
  AudioBusBuffers outputs_buf;

  float *outBuffers[2] = {bufferL, bufferR};
  outputs_buf.numChannels = 2;
  outputs_buf.channelBuffers32 = outBuffers;

  data.numInputs = 0;
  data.inputs = nullptr;
  data.numOutputs = 1;
  data.outputs = &outputs_buf;

  loadProcessDataFromInputEvents(data, eventList, paramChanges, events,
                                 eventCount);

  audioProcessor->process(data);
}

void PluginBridge::subscribeParameterEdit(
    std::function<void(uint32_t paramId, double value)> fn) {
  if (componentHandler) {
    componentHandler->setParameterEditCallback(fn);
  }
}

void PluginBridge::unsubscribeParameterEdit() {
  if (componentHandler) {
    componentHandler->setParameterEditCallback(nullptr);
  }
}

} // namespace vst_dev_host
