#pragma once

#include "../../../api/synthesizer-base.h"
#include "../../../common/listener-port.h"
#include "../../../core/parameter-registry.h"
#include "../../../core/parameter-spec-helper.h"
#include "../../../domain/parameters-store.h"
#include "./interfaces.h"
#include "./parameters-initializer.h"

namespace sonic {

class ParametersHub {
  ParametersStore &parametersStore;

public:
  ParametersHub(ParameterRegistry &parametersRegistry,
                ParametersStore &parametersStore)
      : parametersStore(parametersStore) {}

  SingleListenerPort<ParamId, double, ParameterEditState>
      parameterEditFromUiPort;
  MultipleListenerPort<ParamId, double> parameterChangeFromControllerPort;

  void setParameterFromUi(ParamId id, double value,
                          ParameterEditState editState) {
    if (editState == ParameterEditState::Perform ||
        editState == ParameterEditState::InstantChange) {
      parametersStore.set(id, value);
    } else {
      value = parametersStore.get(id);
    }
    parameterEditFromUiPort.call(id, value, editState);
  }

  void setParameterFromController(ParamId id, double value) {
    parametersStore.set(id, value);
    parameterChangeFromControllerPort.call(id, value);
  }

  double getParameterValue(ParamId id) { return parametersStore.get(id); }
};

class ParameterService {
  ParameterRegistry &parametersRegistry;
  ParametersHub &parametersHub;

public:
  ParameterService(ParameterRegistry &parametersRegistry,
                   ParametersHub &parametersHub)
      : parametersRegistry(parametersRegistry), parametersHub(parametersHub) {}

  int subscribeParameterChanges(
      std::function<void(std::string, double)> listener) {
    int token = parametersHub.parameterChangeFromControllerPort.subscribe(
        [this, listener](int id, double value) {
          auto item = parametersRegistry.getParameterItemById(id);
          if (!item) {
            return;
          }
          auto paramKey = item->paramKey;
          listener(paramKey, value);
        });
    return token;
  }
  void unsubscribeParameterChanges(int token) {
    parametersHub.parameterChangeFromControllerPort.unsubscribe(token);
  }

  void applyParameterEditFromUi(std::string paramKey, double value,
                                ParameterEditState editState) {
    auto idPtr = parametersRegistry.getIdByParamKey(paramKey);
    if (idPtr == std::nullopt) {
      return;
    }
    auto id = *idPtr;
    parametersHub.setParameterFromUi(id, value, editState);
  }
  void getAllParameters(std::map<std::string, double> &parameters) {
    auto parameterItems = parametersRegistry.getParameterItems();
    for (const auto &item : parameterItems) {
      parameters[item.paramKey] = parametersHub.getParameterValue(item.id);
    }
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

  int subscribeParameterChange(
      std::function<void(const std::string, double)> callback) override {
    return parameterService.subscribeParameterChanges(callback);
  }

  void unsubscribeParameterChange(int token) override {
    parameterService.unsubscribeParameterChanges(token);
  }

  void applyParameterEditFromUi(std::string paramKey, double value,
                                ParameterEditState editState) override {
    parameterService.applyParameterEditFromUi(paramKey, value, editState);
  }

  void getAllParameters(std::map<std::string, double> &parameters) override {
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
};

class DomainController {
  SynthesizerBase &synth;
  ParameterRegistry parametersRegistry;
  ParametersStore parametersStore;
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
          // requested note from ui
          // send note request to audio thread via IMessage
          this->messagePort.sendUpstreamEvent(UpstreamEvent{
              .type = UpstreamEventType::NoteRequest,
              .note = {noteNumber, velocity},
          });
          // return host note event to show active notes in ui
          noteService.hostNotePort.call(noteNumber, velocity);
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