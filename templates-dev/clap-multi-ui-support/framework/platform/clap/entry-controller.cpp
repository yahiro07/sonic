#include "./entry-controller.h"
#include "../../api/synthesizer-base.h"
#include "../../common/listener-port.h"
#include "../../common/spsc-queue.h"
#include "../../core/parameter-builder-impl.h"
#include "../../core/parameter-registry.h"
#include "../../core/parameter-spec-helper.h"
#include "../../core/parameter-spec-item.h"
#include "../../core/parameter-store.h"
#include "../../editor/editor-factory-registry.h"
#include "../../editor/interfaces.h"
#include "../../editor/webview/webview-editor.h"
#include "./clap-data-helper.h"
#include "./events.h"
#include <atomic>
#include <cassert>
#include <memory>
#include <string>

namespace sonic {

static UpstreamEventType
mapParameterEditStateToUpstreamEventType(ParameterEditState editState) {
  if (editState == ParameterEditState::Begin) {
    return UpstreamEventType::ParameterBeginEdit;
  } else if (editState == ParameterEditState::Perform) {
    return UpstreamEventType::ParameterApplyEdit;
  } else if (editState == ParameterEditState::End) {
    return UpstreamEventType::ParameterEndEdit;
  } else if (editState == ParameterEditState::InstantChange) {
    return UpstreamEventType::ParameterInstantChange;
  }
  return UpstreamEventType::ParameterApplyEdit;
}

class ParametersHub {
  ParameterStore &parameterStore;

public:
  ParametersHub(ParameterStore &parameterStore)
      : parameterStore(parameterStore) {}

  SingleListenerPort<ParamId, double, ParameterEditState>
      parameterEditFromUiPort;
  MultipleListenerPort<ParamId, double> parameterChangeFromHostPort;

  void setParameterFromUi(ParamId id, double value,
                          ParameterEditState editState) {
    if (editState == ParameterEditState::Perform ||
        editState == ParameterEditState::InstantChange) {
      parameterStore.set(id, value);
    } else {
      value = parameterStore.get(id);
    }
    parameterEditFromUiPort.call(id, value, editState);
  }

  void setParameterFromHost(ParamId id, double value) {
    parameterStore.set(id, value);
    parameterChangeFromHostPort.call(id, value);
  }

  double getParameterValue(ParamId id) { return parameterStore.get(id); }
};

class ParameterService {
  ParameterRegistry &parametersRegistry;
  ParametersHub &parametersHub;

public:
  ParameterService(ParameterRegistry &parametersRegistry,
                   ParametersHub &parametersHub)
      : parametersRegistry(parametersRegistry), parametersHub(parametersHub) {}

  int subscribeParameterChanges(std::function<void(ParamId, double)> listener) {
    int token = parametersHub.parameterChangeFromHostPort.subscribe(
        [this, listener](ParamId id, double value) { listener(id, value); });
    return token;
  }
  void unsubscribeParameterChanges(int token) {
    parametersHub.parameterChangeFromHostPort.unsubscribe(token);
  }

  void applyParameterEditFromUi(ParamId paramId, double value,
                                ParameterEditState editState) {
    parametersHub.setParameterFromUi(paramId, value, editState);
  }
  void getAllParameters(std::map<ParamId, double> &parameters) {
    auto parameterItems = parametersRegistry.getParameterItems();
    for (const auto &item : parameterItems) {
      parameters[item.id] = parametersHub.getParameterValue(item.id);
    }
  }

  std::optional<ParamId> getParameterIdByParamKey(std::string paramKey) {
    auto parameterItems = parametersRegistry.getParameterItems();
    for (const auto &item : parameterItems) {
      if (item.paramKey == paramKey) {
        return item.id;
      }
    }
    return std::nullopt;
  }

  std::optional<std::string> getParameterKeyById(ParamId id) {
    auto parameterItems = parametersRegistry.getParameterItems();
    for (const auto &item : parameterItems) {
      if (item.id == id) {
        return item.paramKey;
      }
    }
    return std::nullopt;
  }
};

class NoteService {
public:
  // note request: DSP <-- Controller <-- UI
  SingleListenerPort<int, double> noteRequestPort;

  // active note state: Host,DSP --> Controller --> UI
  MultipleListenerPort<int, double> hostNotePort;
};

class ControllerFacade : public IControllerFacade {
  ParameterService &parameterService;
  NoteService &noteService;

public:
  ControllerFacade(ParameterService &parameterService, NoteService &noteService)
      : parameterService(parameterService), noteService(noteService) {}

  int subscribeParameterChange(
      std::function<void(ParamId, double)> callback) override {
    return parameterService.subscribeParameterChanges(callback);
  }

  void unsubscribeParameterChange(int token) override {
    parameterService.unsubscribeParameterChanges(token);
  }

  void applyParameterEditFromUi(ParamId paramId, double value,
                                ParameterEditState editState) override {
    parameterService.applyParameterEditFromUi(paramId, value, editState);
  }

  void getAllParameters(std::map<ParamId, double> &parameters) override {
    parameterService.getAllParameters(parameters);
  }

  void requestNoteOn(int noteNumber, double velocity) override {
    noteService.noteRequestPort.call(noteNumber, velocity);
  }
  void requestNoteOff(int noteNumber) override {
    noteService.noteRequestPort.call(noteNumber, .0f);
  }

  int subscribeHostNote(
      std::function<void(int noteNumber, double velocity)> callback) override {
    return noteService.hostNotePort.subscribe(callback);
  }
  void unsubscribeHostNote(int token) override {
    noteService.hostNotePort.unsubscribe(token);
  }

  void incrementViewCount() override {}
  void decrementViewCount() override {}

  std::optional<ParamId>
  getParameterIdByParamKey(std::string paramKey) override {
    return parameterService.getParameterIdByParamKey(paramKey);
  }
  std::optional<std::string> getParameterKeyById(ParamId id) override {
    return parameterService.getParameterKeyById(id);
  }
};

class IHostCallbackRequester {
public:
  virtual ~IHostCallbackRequester() = default;
  virtual void requestMainThreadCallback() = 0;
  virtual void requestFlush() = 0;
};

class HostCallbackRequester : public IHostCallbackRequester {
private:
  const clap_host_t *host;
  const clap_host_params_t *hostParams;

public:
  void initialize(const clap_host_t *host,
                  const clap_host_params_t *hostParams) {
    this->host = host;
    this->hostParams = hostParams;
  }

  void requestMainThreadCallback() override {
    if (host && host->request_callback) {
      host->request_callback(host);
    }
  }

  void requestFlush() override {
    if (host && hostParams && hostParams->request_flush) {
      hostParams->request_flush(host);
    }
  }
};

class IEventBridge {
public:
  virtual void pushUpstreamEvent(UpstreamEvent e) = 0;
  virtual bool popUpstreamEvent(UpstreamEvent &e) = 0;

  virtual void pushDownstreamEvent(DownstreamEvent e) = 0;
  virtual bool popDownstreamEvent(DownstreamEvent &e) = 0;
};

class Eventbridge : public IEventBridge {
private:
  SPSCQueue<UpstreamEvent, 32> upstreamEventQueue;
  SPSCQueue<DownstreamEvent, 32> downstreamEventQueue;

  std::atomic<bool> mainThreadRequestedFlag = false;

  HostCallbackRequester &hostCallbackRequester;

public:
  Eventbridge(HostCallbackRequester &hostCallbackRequester)
      : hostCallbackRequester(hostCallbackRequester) {}

  void pushUpstreamEvent(UpstreamEvent e) override {
    if (upstreamEventQueue.push(e)) {
      hostCallbackRequester.requestFlush();
    }
  }
  bool popUpstreamEvent(UpstreamEvent &e) override {
    return upstreamEventQueue.pop(e);
  }

  void pushDownstreamEvent(DownstreamEvent e) override {
    if (downstreamEventQueue.push(e)) {
      bool expected = false;
      if (mainThreadRequestedFlag.compare_exchange_strong(
              expected, true, std::memory_order_acq_rel)) {
        hostCallbackRequester.requestMainThreadCallback();
      }
    }
  }
  bool popDownstreamEvent(DownstreamEvent &e) override {
    return downstreamEventQueue.pop(e);
  }

  void clearMainThreadRequestedFlag() {
    mainThreadRequestedFlag.store(false, std::memory_order_release);
  }
};

class ProcessorAdapter {
private:
  IPluginSynthesizer &synth;
  IEventBridge &eventBridge;

  void pushDownstreamEvent(DownstreamEvent e) {
    eventBridge.pushDownstreamEvent(e);
  }

  void processInputEvent(const clap_event_header_t *event) {
    if (event->space_id == CLAP_CORE_EVENT_SPACE_ID) {
      if (event->type == CLAP_EVENT_NOTE_ON ||
          event->type == CLAP_EVENT_NOTE_OFF) {
        const clap_event_note_t *noteEvent = (const clap_event_note_t *)event;
        if (event->type == CLAP_EVENT_NOTE_ON) {
          synth.noteOn(noteEvent->key, noteEvent->velocity);
          pushDownstreamEvent({.type = DownstreamEventType::HostNote,
                               .note = {.noteNumber = noteEvent->key,
                                        .velocity = noteEvent->velocity}});
        } else if (event->type == CLAP_EVENT_NOTE_OFF) {
          synth.noteOff(noteEvent->key);
          pushDownstreamEvent(
              {.type = DownstreamEventType::HostNote,
               .note = {.noteNumber = noteEvent->key, .velocity = 0.0}});
        }
      }
      if (event->type == CLAP_EVENT_PARAM_VALUE) {
        auto *paramEvent = (const clap_event_param_value_t *)event;
        auto paramId = paramEvent->param_id;
        auto value = paramEvent->value;
        synth.setParameter(paramId, value);
        pushDownstreamEvent(
            {.type = DownstreamEventType::ParameterChange,
             .param = {.paramId = paramId, .value = (float)value}});
      }
    }
  }

  void drainUpstreamEvents(const clap_output_events_t *out) {
    // outgoing parameters, UI --> Host, DSP
    UpstreamEvent item;
    while (eventBridge.popUpstreamEvent(item)) {
      if (item.type == UpstreamEventType::ParameterBeginEdit) {
        clap_event_param_gesture_t clapEvent{};
        mapUpstreamParamGestureEventToClapEvent(item, clapEvent);
        out->try_push(out, &clapEvent.header);
      } else if (item.type == UpstreamEventType::ParameterApplyEdit) {
        synth.setParameter(item.param.paramId, item.param.value);
        clap_event_param_value_t clapEvent{};
        mapUpstreamParamChangeEventToClapEvent(item, clapEvent);
        out->try_push(out, &clapEvent.header);
      } else if (item.type == UpstreamEventType::ParameterEndEdit) {
        clap_event_param_gesture_t clapEvent{};
        mapUpstreamParamGestureEventToClapEvent(item, clapEvent);
        out->try_push(out, &clapEvent.header);
      } else if (item.type == UpstreamEventType::NoteRequest) {
        if (item.note.velocity > 0.f) {
          synth.noteOn(item.note.noteNumber, item.note.velocity);
        } else {
          synth.noteOff(item.note.noteNumber);
        }
        pushDownstreamEvent({.type = DownstreamEventType::HostNote,
                             .note = {.noteNumber = item.note.noteNumber,
                                      .velocity = item.note.velocity}});
      }
    }
  }

public:
  ProcessorAdapter(IPluginSynthesizer &synth, IEventBridge &eventBridge)
      : synth(synth), eventBridge(eventBridge) {}

  void prepareProcessing(double sampleRate, uint32_t maxFrameCount) {
    synth.prepareProcessing(sampleRate, maxFrameCount);
  }

  void renderAudio(uint32_t start, uint32_t end, float *outputL,
                   float *outputR) {
    synth.processAudio(outputL + start, outputR + start, end - start);
  }

  clap_process_status process(const clap_process_t *process) {
    if (!process)
      return CLAP_PROCESS_ERROR;
    if (process->audio_outputs_count != 1)
      return CLAP_PROCESS_ERROR;

    auto &out = process->audio_outputs[0];
    if (out.data64)
      return CLAP_PROCESS_ERROR;
    if (!out.data32 || out.channel_count < 2 || !out.data32[0] ||
        !out.data32[1])
      return CLAP_PROCESS_ERROR;

    drainUpstreamEvents(process->out_events);

    const uint32_t frameCount = process->frames_count;
    const uint32_t inputEventCount =
        process->in_events ? process->in_events->size(process->in_events) : 0;
    uint32_t eventIndex = 0;
    uint32_t nextEventFrame = inputEventCount ? 0 : frameCount;

    for (uint32_t i = 0; i < frameCount;) {
      while (eventIndex < inputEventCount && nextEventFrame == i) {
        const clap_event_header_t *event =
            process->in_events->get(process->in_events, eventIndex);
        if (!event) {
          eventIndex++;
          continue;
        }

        if (event->time != i) {
          nextEventFrame = event->time;
          break;
        }

        this->processInputEvent(event);
        eventIndex++;

        if (eventIndex == inputEventCount) {
          nextEventFrame = frameCount;
          break;
        }
      }

      this->renderAudio(i, nextEventFrame, out.data32[0], out.data32[1]);
      i = nextEventFrame;
    }

    return CLAP_PROCESS_CONTINUE;
  }

  void flushParameters(const clap_input_events_t *in,
                       const clap_output_events_t *out) {
    if (in) {
      for (uint32_t i = 0; i < in->size(in); i++) {
        const clap_event_header_t *event = in->get(in, i);
        this->processInputEvent(event);
      }
    }

    if (out) {
      drainUpstreamEvents(out);
    }
  }
};

class DomainController {
public:
  IPluginSynthesizer &synth;
  IEventBridge &eventBridge;
  ParameterRegistry parametersRegistry;
  ParameterStore parameterStore; // parameters in main thread
  ParametersHub parametersHub{parameterStore};
  ParameterService parameterService{parametersRegistry, parametersHub};
  NoteService noteService;
  ControllerFacade controllerFacade{parameterService, noteService};

  DomainController(IPluginSynthesizer &synth, IEventBridge &eventBridge)
      : synth(synth), eventBridge(eventBridge) {}

  void initialize(ParameterSpecArray &parameterItems) {
    parametersRegistry.addParameters(parameterItems, 0xFFFFFFFE);

    auto maxId =
        ParameterSpecHelper::getMaxIdFromParameterItems(parameterItems);
    parameterStore.setup(maxId);
    for (const auto &item : parameterItems) {
      parameterStore.set(item.id, item.defaultValue);
    }

    parametersHub.parameterEditFromUiPort.subscribe(
        [this](ParamId id, float value, ParameterEditState editState) {
          auto type = mapParameterEditStateToUpstreamEventType(editState);
          eventBridge.pushUpstreamEvent(
              UpstreamEvent{.type = type, .param = {id, value}});
        });
    noteService.noteRequestPort.subscribe(
        [this](int noteNumber, double velocity) {
          // requested note from ui
          // send note request to audio thread via queue
          eventBridge.pushUpstreamEvent(UpstreamEvent{
              .type = UpstreamEventType::NoteRequest,
              .note = {noteNumber, velocity},
          });
          // return host note event to show active notes in ui
          noteService.hostNotePort.call(noteNumber, velocity);
        });
  }

  void teardown() {
    parametersHub.parameterEditFromUiPort.unsubscribe();
    noteService.noteRequestPort.unsubscribe();
  }

  void onMainThread() {
    DownstreamEvent e;
    while (eventBridge.popDownstreamEvent(e)) {
      if (e.type == DownstreamEventType::ParameterChange) {
        parametersHub.setParameterFromHost(e.param.paramId, e.param.value);
      } else if (e.type == DownstreamEventType::HostNote) {
        noteService.hostNotePort.call(e.note.noteNumber, e.note.velocity);
      }
    }
  }
};

static void registerBuiltinEditorFactories() {
  static bool registered = false;
  if (!registered) {
    registerWebviewEditorFactory();
    registered = true;
  }
}

class EntryControllerImpl : public EntryController {
private:
  std::unique_ptr<IPluginSynthesizer> synth;
  HostCallbackRequester hostCallbackRequester;
  Eventbridge eventBridge{hostCallbackRequester};
  ProcessorAdapter processorAdapter{*synth, eventBridge};
  DomainController domainController{*synth, eventBridge};
  std::unique_ptr<IEditorInstance> editorInstance;

public:
  EntryControllerImpl(IPluginSynthesizer *synth) : synth(std::move(synth)) {}

  void initialize() override {
    printf("EntryControllerImpl::initialize called\n");
    registerBuiltinEditorFactories();

    hostCallbackRequester.initialize(host, hostParams);

    ParameterBuilderImpl builder;
    synth->setupParameters(builder);
    auto parameterItems = builder.getItems();
    domainController.initialize(parameterItems);
  }

  ~EntryControllerImpl() override {
    domainController.teardown();
    guiDestroy();
  }

  void prepareProcessing(double sampleRate, uint32_t maxFrameCount) override {
    processorAdapter.prepareProcessing(sampleRate, maxFrameCount);
  }

  clap_process_status process(const clap_process_t *process) override {
    auto res = processorAdapter.process(process);
    if (res == CLAP_PROCESS_CONTINUE) {
      // telemetrySupport.process();
    }
    return res;
  }

  void flush(const clap_input_events_t *in,
             const clap_output_events_t *out) override {
    processorAdapter.flushParameters(in, out);
  }

  uint32_t getParameterCount() const override {
    return domainController.parametersRegistry.getParameterItems().size();
  }

  void getParameterInfo(uint32_t index,
                        clap_param_info_t *info) const override {
    auto parameterItems =
        domainController.parametersRegistry.getParameterItems();
    auto &item = parameterItems[index];
    assignParameterInfo(info, item);
  }

  double getParameterValue(clap_id id) override {
    return domainController.parameterStore.get(id);
  }

  void onTimer(clap_id timerId) override {
    // printf("onTimer called, timerId: %d\n", timerId);
    // if (timerId == telemetryUpdateTimerId) {
    //   telemetrySupport.updateTelemetries();
    // }
  }

  void onMainThread() override {
    eventBridge.clearMainThreadRequestedFlag();
    domainController.onMainThread();
  }

  bool guiCreate() override {
    std::string url = synth->getEditorPageUrl();
    auto variantName = url.substr(0, url.find(":"));
    if (variantName == "http" || variantName == "https") {
      variantName = "webview";
    }
    printf("guiCreate called, variantName: %s\n", variantName.c_str());

    auto editorFactory =
        EditorFactoryRegistry::getInstance()->getEditorFactory(variantName);
    if (!editorFactory) {
      printf("editor factory not found for variant: %s\n", variantName.c_str());
      return false;
    }
    editorInstance = editorFactory(domainController.controllerFacade);
    if (variantName == "webview") {
      editorInstance->setup(url);
    } else {
      auto loadTargetSpec = url.substr(url.find(":") + 1);
      editorInstance->setup(loadTargetSpec);
    }
    return true;
  }

  void guiDestroy() override { editorInstance->teardown(); }

  bool guiSetParent(const clap_window_t *window) override {
    if (!editorInstance) {
      return false;
    }
    assert(0 == std::strcmp(window->api, CLAP_WINDOW_API_COCOA));
    editorInstance->attachToParent(window->cocoa);
    return true;
  }

  bool guiSetSize(uint32_t width, uint32_t height) override {
    if (!editorInstance) {
      return false;
    }
    editorInstance->setFrame(0, 0, width, height);
    return true;
  }

  bool guiShow() override { return true; }

  bool guiHide() override { return true; }
};

EntryController *EntryController::create(void *synthInstance) {
  return new EntryControllerImpl(
      static_cast<sonic::IPluginSynthesizer *>(synthInstance));
}

} // namespace sonic