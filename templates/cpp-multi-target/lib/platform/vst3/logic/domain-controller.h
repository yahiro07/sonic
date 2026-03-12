#pragma once

#include "../../../api/synthesizer-base.h"
#include "../../../common/listener-port.h"
#include "../../../core/parameter-builder-impl.h"
#include "../../../core/parameter-registry.h"
#include "../../../core/parameter-spec-helper.h"
#include "../../../domain/parameters-store.h"

#include "./interfaces.h"

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

public:
  ControllerFacade(ParameterService &parameterService, NoteService &noteService)
      : parameterService(parameterService), noteService(noteService) {}

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
  IUpstreamEventBus &upstreamEventBus;
  IDownstreamEventBus &downstreamEventBus;
  ParameterService parameterService{parametersRegistry, parametersHub};
  ControllerFacade controllerFacade{parameterService, noteService};

public:
  DomainController(SynthesizerBase &synth,
                   IControllerParameterPortal &controllerParameterPortal,
                   IUpstreamEventBus &upstreamEventBus,
                   IDownstreamEventBus &downstreamEventBus)
      : synth(synth), controllerParameterPortal(controllerParameterPortal),
        upstreamEventBus(upstreamEventBus),
        downstreamEventBus(downstreamEventBus) {
    auto parameterBuilder = ParameterBuilderImpl();
    synth.setupParameters(parameterBuilder);
    auto parameterItems = parameterBuilder.getItems();
    parametersRegistry.addParameters(parameterItems, 0x7FFFFFFE);
    auto maxId =
        ParameterSpecHelper::getMaxIdFromParameterItems(parameterItems);
    parametersStore.setup(maxId);
    for (const auto &item : parameterItems) {
      parametersStore.set(item.id, item.defaultValue);
    }

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
              std::to_string(id), normalizedValue, editState);
        });

    noteService.noteRequestPort.subscribe(
        [this](int noteNumber, double velocity) {
          // requested note from ui
          // send note request to audio thread via IMessage
          this->upstreamEventBus.pushUpstreamEvent(UpstreamEvent{
              .type = UpstreamEventType::NoteRequest,
              .note = {noteNumber, velocity},
          });
          // return host note event to show active notes in ui
          noteService.hostNotePort.call(noteNumber, velocity);
        });
  }

  void teardown() {
    controllerParameterPortal.unsubscribeParameterChange();
    parametersHub.parameterEditFromUiPort.unsubscribe();
    noteService.noteRequestPort.unsubscribe();
  }

  void onMatinThreadTimer() {
    DownstreamEvent e;
    while (downstreamEventBus.popDownstreamEvent(e)) {
      if (e.type == DownstreamEventType::HostNote) {
        noteService.hostNotePort.call(e.note.noteNumber, e.note.velocity);
      }
    }
  }

  IControllerFacade &getControllerFacade() { return controllerFacade; }
};

} // namespace sonic