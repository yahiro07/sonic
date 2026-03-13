#include "plugin_bridge.h"
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include <cstdio>

namespace vst_dev_host {

using namespace Steinberg;
using namespace Steinberg::Vst;

static inline bool isOk(tresult r) {
  return (r == kResultTrue) || (r == kResultOk);
}

class HostPlugFrame final : public Steinberg::IPlugFrame {
public:
  HostPlugFrame() { FUNKNOWN_CTOR }
  ~HostPlugFrame() { FUNKNOWN_DTOR }

  void setResizeRequestCallback(
      std::function<bool(int width, int height)> callback) {
    resizeRequestCallback = std::move(callback);
  }

  void clearResizeRequestCallback() { resizeRequestCallback = nullptr; }

  tresult PLUGIN_API resizeView(Steinberg::IPlugView *view,
                                Steinberg::ViewRect *newSize) override {
    if (!newSize) {
      return kResultFalse;
    }

    const int width = (int)(newSize->right - newSize->left);
    const int height = (int)(newSize->bottom - newSize->top);

    if (!resizeRequestCallback) {
      return kResultFalse;
    }

    const bool accepted = resizeRequestCallback(width, height);

    if (!accepted) {
      return kResultFalse;
    }

    // Ensure the plugin view is informed about the new size.
    if (view) {
      (void)view->onSize(newSize);
    }
    return kResultTrue;
  }

  DECLARE_FUNKNOWN_METHODS

private:
  std::function<bool(int width, int height)> resizeRequestCallback = nullptr;
};
IMPLEMENT_FUNKNOWN_METHODS(HostPlugFrame, IPlugFrame, IPlugFrame::iid)

class PluginBridge::ComponentHandler
    : public Steinberg::Vst::IComponentHandler {
public:
  ComponentHandler() { FUNKNOWN_CTOR }
  ~ComponentHandler(){FUNKNOWN_DTOR}

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

void PluginBridge::getDesiredEditorSize(int &width, int &height) {
  width = 0;
  height = 0;
  if (!editController) {
    return;
  }

  Steinberg::IPlugView *view = plugView;
  bool temporary = false;
  if (!view) {
    view = editController->createView(ViewType::kEditor);
    temporary = (view != nullptr);
  }

  if (!view) {
    return;
  }

  ViewRect rect;
  if (isOk(view->getSize(&rect))) {
    width = (int)(rect.right - rect.left);
    height = (int)(rect.bottom - rect.top);
  }

  if (temporary) {
    view->release();
  }
}

void PluginBridge::openEditor(void *ownerViewHandle) {
  createEditor(ownerViewHandle);
}

bool PluginBridge::requestEditorResize(int &width, int &height) {
  if (!plugView) {
    return false;
  }

  // If the plugin cannot resize, keep the current size.
  if (!isOk(plugView->canResize())) {
    ViewRect current;
    if (isOk(plugView->getSize(&current))) {
      width = (int)(current.right - current.left);
      height = (int)(current.bottom - current.top);
    }
    return false;
  }

  ViewRect rect;
  rect.left = 0;
  rect.top = 0;
  rect.right = width;
  rect.bottom = height;

  // Let the plugin clamp/adjust sizes if it wants.
  if (isOk(plugView->checkSizeConstraint(&rect))) {
    width = (int)(rect.right - rect.left);
    height = (int)(rect.bottom - rect.top);
  }

  const auto r = plugView->onSize(&rect);
  if (!isOk(r)) {
    return false;
  }

  width = (int)(rect.right - rect.left);
  height = (int)(rect.bottom - rect.top);
  return true;
}

void PluginBridge::subscribeEditorSizeChangeRequest(
    std::function<bool(int width, int height)> callback) {
  editorSizeChangeRequestCallback = callback;

  // Ensure we have a frame to receive resize requests.
  if (!plugFrame) {
    hostPlugFrame = new HostPlugFrame();
    plugFrame = Steinberg::IPtr<Steinberg::IPlugFrame>(hostPlugFrame, false);
  }

  if (hostPlugFrame) {
    hostPlugFrame->setResizeRequestCallback(editorSizeChangeRequestCallback);
  }

  if (plugView) {
    (void)plugView->setFrame(plugFrame);
  }
}

void PluginBridge::unsubscribeEditorSizeChangeRequest() {
  editorSizeChangeRequestCallback = nullptr;
  if (hostPlugFrame) {
    hostPlugFrame->clearResizeRequestCallback();
  }
}

static bool connectComponentAndController(IComponent *component,
                                          IEditController *controller) {
  FUnknownPtr<IConnectionPoint> componentCP(component);
  FUnknownPtr<IConnectionPoint> controllerCP(controller);

  if (!componentCP || !controllerCP) {
    return false;
  }
  const auto r1 = componentCP->connect(controllerCP);
  const auto r2 = controllerCP->connect(componentCP);

  return (r1 == kResultOk || r1 == kResultTrue) &&
         (r2 == kResultOk || r2 == kResultTrue);
}
static bool disconnectComponentAndController(IComponent *component,
                                             IEditController *controller) {
  FUnknownPtr<IConnectionPoint> componentCP(component);
  FUnknownPtr<IConnectionPoint> controllerCP(controller);

  if (!componentCP || !controllerCP) {
    return false;
  }

  const auto r1 = componentCP->disconnect(controllerCP);
  const auto r2 = controllerCP->disconnect(componentCP);

  return (r1 == kResultOk || r1 == kResultTrue) &&
         (r2 == kResultOk || r2 == kResultTrue);
}

bool PluginBridge::loadPlugin(const std::string &path) {
  printf("PluginBridge::loadPlugin: %s\n", path.c_str());

  unloadPlugin();

  std::string errorDescription;
  module = VST3::Hosting::Module::create(path, errorDescription);
  if (!module) {
    printf("Failed to load module: %s\n", errorDescription.c_str());
    return false;
  }

  VST3::Hosting::PluginFactory factory = module->getFactory();
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
    return false;
  }

  isConnected = false;
  isActive = false;
  isProcessing = false;
  controllerIsComponent = false;

  component = factory.createInstance<IComponent>(audioEffectClassInfo.ID());
  if (!component) {
    printf("Failed to create component instance\n");
    return false;
  }

  {
    FUnknownPtr<IPluginBase> plugBase(component);
    if (!plugBase) {
      printf("Component does not support IPluginBase\n");
    } else {
      const auto initResult = plugBase->initialize(&hostApp);
      if (initResult != kResultOk && initResult != kResultTrue) {
        printf("Component initialize failed: %d\n",
               static_cast<int>(initResult));
      }
    }
  }

  audioProcessor = FUnknownPtr<IAudioProcessor>(component);
  if (!audioProcessor) {
    printf("Component does not support IAudioProcessor\n");
  }

  // Controller: either the component is also a controller, or we create one
  // using the component-provided controller class ID.
  {
    FUnknownPtr<IEditController> controllerFromComponent(component);
    if (controllerFromComponent) {
      controllerIsComponent = true;
      editController = controllerFromComponent;
    } else {
      TUID controllerCID;
      if (component->getControllerClassId(controllerCID) == kResultTrue) {
        editController =
            factory.createInstance<IEditController>(VST3::UID(controllerCID));
        if (!editController) {
          printf("Failed to create edit controller instance\n");
        } else {
          FUnknownPtr<IPluginBase> plugBase(editController);
          if (!plugBase) {
            printf("Controller does not support IPluginBase\n");
          } else {
            const auto initResult = plugBase->initialize(&hostApp);
            if (initResult != kResultOk && initResult != kResultTrue) {
              printf("Controller initialize failed: %d\n",
                     static_cast<int>(initResult));
            }
          }
        }
      } else {
        printf(
            "Component did not provide controller class ID (no controller)\n");
      }
    }
  }

  componentHandler = IPtr<ComponentHandler>(new ComponentHandler(), false);
  if (editController) {
    editController->setComponentHandler(componentHandler);
  }

  if (component && editController && !controllerIsComponent) {
    isConnected = connectComponentAndController(component, editController);
  }

  return true;
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

  // Provide an IPlugFrame so plugins can request host resizes.
  if (!plugFrame) {
    hostPlugFrame = new HostPlugFrame();
    plugFrame = Steinberg::IPtr<Steinberg::IPlugFrame>(hostPlugFrame, false);
  }

  if (hostPlugFrame) {
    hostPlugFrame->setResizeRequestCallback(editorSizeChangeRequestCallback);
  }
  (void)plugView->setFrame(plugFrame);

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
    (void)plugView->setFrame(nullptr);
    plugView->removed();
    plugView->release();
    plugView = nullptr;
  }
}

void PluginBridge::unloadPlugin() {
  if (!module && !component && !audioProcessor && !editController &&
      !plugView && !componentHandler) {
    return;
  }

  printf("Unloading plugin\n");
  closeEditor();

  // Stop audio processing first.
  if (audioProcessor && isProcessing) {
    const auto r = audioProcessor->setProcessing(false);
    (void)r;
    isProcessing = false;
  }

  // Deactivate while controller/connection is still present.
  if (component && isActive) {
    const auto r = component->setActive(false);
    (void)r;
    isActive = false;
  }

  // Now it is safe to disconnect.
  if (component && editController && !controllerIsComponent && isConnected) {
    (void)disconnectComponentAndController(component, editController);
    isConnected = false;
  }

  // Detach host handler before termination.
  if (editController) {
    editController->setComponentHandler(nullptr);
  }
  if (componentHandler) {
    componentHandler->clearParameterEditCallback();
  }

  // Terminate in the same order as the SDK helper (component first).
  if (component) {
    FUnknownPtr<IPluginBase> plugBase(component);
    if (plugBase) {
      plugBase->terminate();
    }
  }
  if (editController && !controllerIsComponent) {
    FUnknownPtr<IPluginBase> plugBase(editController);
    if (plugBase) {
      plugBase->terminate();
    }
  }

  component = nullptr;
  audioProcessor = nullptr;
  editController = nullptr;
  module = nullptr;
  componentHandler = nullptr;
  editorSizeChangeRequestCallback = nullptr;
  if (hostPlugFrame) {
    hostPlugFrame->clearResizeRequestCallback();
  }
  plugFrame = nullptr;
  hostPlugFrame = nullptr;
  controllerIsComponent = false;
  isConnected = false;
  isActive = false;
  isProcessing = false;
  printf("Unloading plugin...done\n");
}

void PluginBridge::prepareAudio(double sampleRate, int maxBlockSize) {
  if (audioProcessor) {
    if (component) {
      const auto r = component->setActive(true);
      (void)r;
      isActive = true;
    }

    ProcessSetup setup;
    setup.processMode = kRealtime;
    setup.symbolicSampleSize = kSample32;
    setup.maxSamplesPerBlock = maxBlockSize;
    setup.sampleRate = sampleRate;

    Vst::SpeakerArrangement stereo = Vst::SpeakerArr::kStereo;
    audioProcessor->setBusArrangements(&stereo, 1, &stereo, 1);

    audioProcessor->setupProcessing(setup);
    audioProcessor->setProcessing(true);
    isProcessing = true;

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
