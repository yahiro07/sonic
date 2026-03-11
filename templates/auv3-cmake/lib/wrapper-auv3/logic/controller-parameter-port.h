#pragma once

#include "../core/parameter-definitions-provider.h"
#include "../domain/interfaces.h"
#include "../support/parameter-tree-wrapper.h"
#include <map>

namespace sonic {

class ControllerParameterPort {
private:
  ParameterTreeWrapper &_parameterTreeWrapper;
  ParameterDefinitionsProvider &_parametersDefinitionProvider;
  void *ptObserverToken = nullptr;

  std::map<int, std::function<void(std::string, float)>> listeners;
  int nextListenerToken = 0;

  void startObserve() {

    ptObserverToken = _parameterTreeWrapper.tokenByAddingParameterObserver(
        [this](uint64_t address, float value) {
          auto id = (int32_t)address;
          auto item = _parametersDefinitionProvider.getParameterItemById(id);
          if (!item) {
            return;
          }
          auto paramKey = item->paramKey;
          for (const auto &[token, listener] : listeners) {
            listener(paramKey, value);
          }
        });
    printf("startObserve, observerToken: %p\n", ptObserverToken);
  }

  void stopObserve() {
    if (ptObserverToken) {
      _parameterTreeWrapper.removeParameterObserver(ptObserverToken);
      ptObserverToken = nullptr;
    }
  }

public:
  ControllerParameterPort(
      ParameterTreeWrapper &parameterTreeWrapper,
      ParameterDefinitionsProvider &parametersDefinitionProvider)
      : _parameterTreeWrapper(parameterTreeWrapper),
        _parametersDefinitionProvider(parametersDefinitionProvider) {}

  ~ControllerParameterPort() { stopObserve(); }

  int subscribeToParameterChanges(
      std::function<void(std::string, double)> listener) {
    int token = nextListenerToken++;
    listeners[token] = listener;
    if (listeners.size() == 1) {
      startObserve();
    }
    return token;
  }

  void unsubscribeFromParameterChanges(int token) {
    listeners.erase(token);
    if (listeners.empty()) {
      stopObserve();
    }
  }

  void applyParameterEditFromUi(std::string paramKey, float value,
                                ParameterEditState editState) {
    auto idPtr = _parametersDefinitionProvider.getIdByParamKey(paramKey);
    if (idPtr == std::nullopt) {
      return;
    }
    auto id = *idPtr;
    if (editState == ParameterEditState::Begin) {
      auto currentValue = _parameterTreeWrapper.getParameterValue(id);
      _parameterTreeWrapper.setParameterValue(
          id, currentValue, ptObserverToken,
          ParameterAutomationEventType::Touch);
    } else if (editState == ParameterEditState::End) {
      auto currentValue = _parameterTreeWrapper.getParameterValue(id);
      _parameterTreeWrapper.setParameterValue(
          id, currentValue, ptObserverToken,
          ParameterAutomationEventType::Release);
    } else if (editState == ParameterEditState::Perform) {
      _parameterTreeWrapper.setParameterValue(
          id, value, ptObserverToken, ParameterAutomationEventType::Value);
    } else if (editState == ParameterEditState::InstantChange) {
      _parameterTreeWrapper.setParameterValue(
          id, value, ptObserverToken, ParameterAutomationEventType::Touch);
      _parameterTreeWrapper.setParameterValue(
          id, value, ptObserverToken, ParameterAutomationEventType::Value);
      _parameterTreeWrapper.setParameterValue(
          id, value, ptObserverToken, ParameterAutomationEventType::Release);
    }
  }

  void getAllParameters(std::map<std::string, float> &parameters) {
    auto parameterItems = _parametersDefinitionProvider.getParameterItems();
    for (const auto &item : parameterItems) {
      auto value = parameters[item.paramKey] =
          _parameterTreeWrapper.getParameterValue(item.id);
    }
  }
};

} // namespace sonic