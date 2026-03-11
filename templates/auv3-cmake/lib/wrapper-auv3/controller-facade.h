#pragma once
#include "../domain/interfaces.h"
#include "./controller-parameter-port.h"

namespace sonic {

template <typename... Args> class SingleListenerEventPort {
private:
  std::function<void(Args...)> listener;

public:
  void subscribe(std::function<void(Args...)> listener) {
    this->listener = listener;
  }
  void unsubscribe() { this->listener = nullptr; }

  void call(Args... args) {
    if (listener) {
      listener(args...);
    }
  }
};

class ControllerFacade : public IControllerFacade {
private:
  ControllerParameterPort &parameterPort;

public:
  SingleListenerEventPort<int, float> noteRequestedPort;

  ControllerFacade(ControllerParameterPort &parameterPort)
      : parameterPort(parameterPort) {}

  int subscribeParameterChange(
      std::function<void(const std::string, float)> callback) override {
    return parameterPort.subscribeToParameterChanges(callback);
  }

  void unsubscribeParameterChange(int token) override {
    parameterPort.unsubscribeFromParameterChanges(token);
  }

  void applyParameterEditFromUi(std::string paramKey, float value,
                                ParameterEditState editState) override {
    parameterPort.applyParameterEditFromUi(paramKey, value, editState);
  }

  void getAllParameters(std::map<std::string, float> &parameters) override {
    parameterPort.getAllParameters(parameters);
  }

  void requestNoteOn(int noteNumber, float velocity) override {
    noteRequestedPort.call(noteNumber, velocity);
  }
  void requestNoteOff(int noteNumber) override {
    noteRequestedPort.call(noteNumber, .0f);
  }
};

} // namespace sonic