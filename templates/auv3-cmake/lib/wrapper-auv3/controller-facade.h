#pragma once
#include "../domain/interfaces.h"
#include "./controller-parameter-port.h"

namespace sonic {
class ControllerFacade : public IControllerFacade {
private:
  ControllerParameterPort &parameterPort;

public:
  ControllerFacade(ControllerParameterPort &parameterPort)
      : parameterPort(parameterPort) {}

  int subscribeParameterChange(
      std::function<void(const std::string, double)> callback) override {
    return parameterPort.subscribeToParameterChanges(callback);
  }

  void unsubscribeParameterChange(int token) override {
    parameterPort.unsubscribeFromParameterChanges(token);
  }

  void applyParameterEditFromUi(std::string paramKey, double value,
                                ParameterEditState editState) override {
    parameterPort.applyParameterEditFromUi(paramKey, value, editState);
  }

  void getAllParameters(std::map<std::string, double> &parameters) override {
    parameterPort.getAllParameters(parameters);
  }

  void requestNoteOn(int noteNumber, double velocity) override {}
  void requestNoteOff(int noteNumber) override {}
};
} // namespace sonic