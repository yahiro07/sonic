#pragma once

#include "./interfaces.h"
#include "./parameters-initializer.h"
#include <sonic/api/synthesizer-base.h>
#include <sonic/common/listener-port.h>
#include <sonic/core/parameter-registry.h>
#include <sonic/core/parameter-spec-helper.h>
#include <sonic/core/parameter-store.h>

namespace sonic {

class ParametersHub {
  ParameterStore &parameterStore;

public:
  ParametersHub(ParameterRegistry &parametersRegistry,
                ParameterStore &parameterStore)
      : parameterStore(parameterStore) {}

  SingleListenerPort<ParamId, double, ParameterEditState>
      parameterEditFromUiPort;
  MultipleListenerPort<ParamId, double> parameterChangeFromControllerPort;

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

  void setParameterFromController(ParamId id, double value) {
    parameterStore.set(id, value);
    parameterChangeFromControllerPort.call(id, value);
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
    int token = parametersHub.parameterChangeFromControllerPort.subscribe(
        [this, listener](int id, double value) { listener(id, value); });
    return token;
  }
  void unsubscribeParameterChanges(int token) {
    parametersHub.parameterChangeFromControllerPort.unsubscribe(token);
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

  std::optional<std::string> getParameterKeyById(ParamId id) {
    return parametersRegistry.getParamKeyById(id);
  }

  std::optional<ParamId> getParameterIdByParamKey(std::string paramKey) {
    return parametersRegistry.getIdByParamKey(paramKey);
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

class ParameterNormalizationConverter {
private:
  ParameterRegistry &parametersRegistry;

public:
  ParameterNormalizationConverter(ParameterRegistry &parametersRegistry)
      : parametersRegistry(parametersRegistry) {}

  double normalizeValue(ParamId id, double value) {
    auto item = parametersRegistry.getParameterItemById(id);
    if (!item) {
      return value;
    }
    return ParameterSpecHelper::getNormalized(item, value);
  }

  double unnormalizeValue(ParamId id, double value) {
    auto item = parametersRegistry.getParameterItemById(id);
    if (!item) {
      return value;
    }
    return ParameterSpecHelper::getUnnormalized(item, value);
  }
};

class ControllerFacade : public IControllerFacade {
  ParameterService &parameterService;
  NoteService &noteService;
  IMainLoopTimer &mainLoopTimer;

  int viewCount = 0;

public:
  ControllerFacade(ParameterService &parameterService, NoteService &noteService,
                   IMainLoopTimer &mainLoopTimer)
      : parameterService(parameterService), noteService(noteService),
        mainLoopTimer(mainLoopTimer) {}

  ControllerFacade(const ControllerFacade &) = delete;
  ControllerFacade &operator=(const ControllerFacade &) = delete;
  ControllerFacade(ControllerFacade &&) = delete;
  ControllerFacade &operator=(ControllerFacade &&) = delete;

  int subscribeParameterChange(
      std::function<void(ParamId, double)> callback) override {
    return parameterService.subscribeParameterChanges(callback);
  }

  void unsubscribeParameterChange(int token) override {
    parameterService.unsubscribeParameterChanges(token);
  }

  void applyParameterEditFromUi(ParamId id, double value,
                                ParameterEditState editState) override {
    parameterService.applyParameterEditFromUi(id, value, editState);
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

  void incrementViewCount() override {
    viewCount++;
    if (viewCount == 1) {
      mainLoopTimer.start();
    }
  }

  void decrementViewCount() override {
    viewCount--;
    if (viewCount == 0) {
      mainLoopTimer.stop();
    }
  }

  std::optional<std::string> getParameterKeyById(ParamId id) override {
    return parameterService.getParameterKeyById(id);
  }
  std::optional<ParamId>
  getParameterIdByParamKey(std::string paramKey) override {
    return parameterService.getParameterIdByParamKey(paramKey);
  }

  const ParameterSpecArray &getParameterSpecs() override {
    return parameterService.getParameterSpecs();
  }
};

class DomainController {
  SynthesizerBase &synth;
  ParameterRegistry parametersRegistry;
  ParameterStore parametersStore;
  IControllerParameterPortal &controllerParameterPortal;
  ParametersHub parametersHub{parametersRegistry, parametersStore};
  ParameterNormalizationConverter parameterNormalizationConverter{
      parametersRegistry};
  NoteService noteService;
  IControllerSideMessagePort &messagePort;
  ParameterService parameterService{parametersRegistry, parametersHub};
  IMainLoopTimer &mainLoopTimer;
  ControllerFacade controllerFacade{parameterService, noteService,
                                    mainLoopTimer};

public:
  DomainController(SynthesizerBase &synth,
                   IControllerParameterPortal &controllerParameterPortal,
                   IControllerSideMessagePort &messagePort,
                   IMainLoopTimer &mainLoopTimer)
      : synth(synth), controllerParameterPortal(controllerParameterPortal),
        messagePort(messagePort), mainLoopTimer(mainLoopTimer) {
    initializeParameters(synth, parametersRegistry, parametersStore);

    // parameter flow: Controller <-- (normalize) <-- Hub <-- UI
    controllerParameterPortal.subscribeParameterChange(
        [this](ParamId id, double normalizedValue) {
          auto value = parameterNormalizationConverter.unnormalizeValue(
              id, normalizedValue);
          this->parametersHub.setParameterFromController(id, value);
        });

    // parameter flow: Controller --> (unnormalize) --> Hub --> UI
    parametersHub.parameterEditFromUiPort.subscribe(
        [this](ParamId id, double unnormalizedValue,
               ParameterEditState editState) {
          auto normalizedValue = parameterNormalizationConverter.normalizeValue(
              id, unnormalizedValue);
          this->controllerParameterPortal.applyParameterEdit(
              id, normalizedValue, editState);
        });

    noteService.noteRequestPort.subscribe(
        [this](int noteNumber, double velocity) {
          this->messagePort.sendUpstreamEvent(UpstreamEvent{
              .type = UpstreamEventType::NoteRequest,
              .note = {noteNumber, velocity},
          });
        });

    mainLoopTimer.setCallback([this]() { this->onMatinThreadTimer(); });
  }

  void teardown() {
    mainLoopTimer.clearCallback();
    controllerParameterPortal.unsubscribeParameterChange();
    parametersHub.parameterEditFromUiPort.unsubscribe();
    noteService.noteRequestPort.unsubscribe();
  }

  void onMatinThreadTimer() {
    DownstreamEvent e;
    while (messagePort.popDownstreamEventReceived(e)) {
      if (e.type == DownstreamEventType::HostNote) {
        noteService.hostNotePort.call(e.note.noteNumber, e.note.velocity);
      }
    }
    messagePort.sendUpstreamEvent(UpstreamEvent{
        .type = UpstreamEventType::PollingProcessorSideEvent,
    });
  }

  ControllerFacade &getControllerFacade() { return controllerFacade; }
};

} // namespace sonic