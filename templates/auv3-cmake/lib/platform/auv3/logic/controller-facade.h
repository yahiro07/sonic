#pragma once
#include "../../../common/listener-port.h"
#include "../../../domain/interfaces.h"
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
      std::function<void(const std::string, float)> callback) override {
    return parameterService.subscribeToParameterChanges(callback);
  }

  void unsubscribeParameterChange(int token) override {
    parameterService.unsubscribeFromParameterChanges(token);
  }

  void applyParameterEditFromUi(std::string paramKey, float value,
                                ParameterEditState editState) override {
    parameterService.applyParameterEditFromUi(paramKey, value, editState);
  }

  void getAllParameters(std::map<std::string, float> &parameters) override {
    parameterService.getAllParameters(parameters);
  }

  void requestNoteOn(int noteNumber, float velocity) override {
    noteService.noteRequestPort.call(noteNumber, velocity);
  }
  void requestNoteOff(int noteNumber) override {
    noteService.noteRequestPort.call(noteNumber, .0f);
  }

  int subscribeHostNote(
      std::function<void(int noteNumber, float velocity)> callback) override {
    return noteService.hostNotePort.subscribe(callback);
  }
  void unsubscribeHostNote(int token) override {
    noteService.hostNotePort.unsubscribe(token);
  }
};

} // namespace sonic