#pragma once
#include "../../../common/listener-port.h"
#include "../../../core/editor-interfaces.h"
#include "./note-service.h"
#include "./parameters-service.h"

namespace sonic {

class ControllerFacade : public IControllerFacade {
private:
  ParameterService &parameterService;
  NoteService &noteService;

public:
  ControllerFacade(ParameterService &parameterService, NoteService &noteService)
      : parameterService(parameterService), noteService(noteService) {}

  int subscribeParameterChange(
      std::function<void(ParamId paramId, double)> callback) override {
    return parameterService.subscribeToParameterChanges(callback);
  }

  void unsubscribeParameterChange(int token) override {
    parameterService.unsubscribeFromParameterChanges(token);
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

} // namespace sonic