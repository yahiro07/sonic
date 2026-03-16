#pragma once
#include "../../common/listener-port.h"
#include "../../common/spsc-queue.h"
#include "../../core/editor-interfaces.h"
#include "../../core/parameter-registry.h"
#include "../../core/parameter-spec-helper.h"
#include "../../core/parameter-store.h"
#include "./events.h"
#include "./interfaces.h"

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

  const ParameterSpecArray &getParameterSpecs() {
    return parametersRegistry.getParameterItems();
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

  const ParameterSpecArray &getParameterSpecs() override {
    return parameterService.getParameterSpecs();
  }
};

class Eventbridge : public IEventBridge {
private:
  SPSCQueue<UpstreamEvent, 32> upstreamEventQueue;
  SPSCQueue<DownstreamEvent, 32> downstreamEventQueue;

  std::atomic<bool> mainThreadRequestedFlag = false;

  IHostCallbackRequester &hostCallbackRequester;

public:
  Eventbridge(IHostCallbackRequester &hostCallbackRequester)
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

} // namespace sonic