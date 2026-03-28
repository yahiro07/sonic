#pragma once

#include "../support/parameter-tree-wrapper.h"
#include <map>
#include <sonic/core/editor-interfaces.h>
#include <sonic/core/parameter-registry.h>

namespace sonic {

class ParameterService {
private:
  ParameterTreeWrapper &parameterTreeWrapper;
  ParameterRegistry &parameterRegistry;
  void *ptObserverToken = nullptr;

  std::map<int, std::function<void(ParamId, double)>> listeners;
  int nextListenerToken = 0;

  void startObserve() {

    ptObserverToken = parameterTreeWrapper.tokenByAddingParameterObserver(
        [this](uint64_t address, double value) {
          auto paramId = (int32_t)address;
          for (const auto &[token, listener] : listeners) {
            listener(paramId, value);
          }
        });
    printf("startObserve, observerToken: %p\n", ptObserverToken);
  }

  void stopObserve() {
    if (ptObserverToken) {
      parameterTreeWrapper.removeParameterObserver(ptObserverToken);
      ptObserverToken = nullptr;
    }
  }

public:
  ParameterService(ParameterTreeWrapper &parameterTreeWrapper,
                   ParameterRegistry &parameterRegistry)
      : parameterTreeWrapper(parameterTreeWrapper),
        parameterRegistry(parameterRegistry) {}

  ~ParameterService() { stopObserve(); }

  int subscribeToParameterChanges(
      std::function<void(ParamId, double)> listener) {
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

  void applyParameterEditFromUi(ParamId id, double value,
                                ParameterEditState editState) {
    if (editState == ParameterEditState::Begin) {
      auto currentValue = parameterTreeWrapper.getParameterValue(id);
      parameterTreeWrapper.setParameterValue(
          id, currentValue, ptObserverToken,
          ParameterAutomationEventType::Touch);
    } else if (editState == ParameterEditState::End) {
      auto currentValue = parameterTreeWrapper.getParameterValue(id);
      parameterTreeWrapper.setParameterValue(
          id, currentValue, ptObserverToken,
          ParameterAutomationEventType::Release);
    } else if (editState == ParameterEditState::Perform) {
      parameterTreeWrapper.setParameterValue(
          id, value, ptObserverToken, ParameterAutomationEventType::Value);
    } else if (editState == ParameterEditState::InstantChange) {
      parameterTreeWrapper.setParameterValue(
          id, value, ptObserverToken, ParameterAutomationEventType::Touch);
      parameterTreeWrapper.setParameterValue(
          id, value, ptObserverToken, ParameterAutomationEventType::Value);
      parameterTreeWrapper.setParameterValue(
          id, value, ptObserverToken, ParameterAutomationEventType::Release);
    }
  }

  void getAllParameters(std::map<ParamId, double> &parameters) {
    auto parameterItems = parameterRegistry.getParameterItems();
    for (const auto &item : parameterItems) {
      auto value = parameters[item.id] =
          parameterTreeWrapper.getParameterValue(item.id);
    }
  }

  std::optional<ParamId> getParameterIdByParamKey(std::string paramKey) {
    auto parameterItems = parameterRegistry.getParameterItems();
    for (const auto &item : parameterItems) {
      if (item.paramKey == paramKey) {
        return item.id;
      }
    }
    return std::nullopt;
  }

  std::optional<std::string> getParameterKeyById(ParamId id) {
    auto parameterItems = parameterRegistry.getParameterItems();
    for (const auto &item : parameterItems) {
      if (item.id == id) {
        return item.paramKey;
      }
    }
    return std::nullopt;
  }

  const ParameterSpecArray &getParameterSpecs() {
    return parameterRegistry.getParameterItems();
  }

  void getAllParametersForPersist(std::map<std::string, double> &parameters) {
    auto parameterItems = parameterRegistry.getParameterItems();
    for (const auto &item : parameterItems) {
      parameters[item.paramKey] =
          parameterTreeWrapper.getParameterValue(item.id);
    }
  }
  void
  setAllParametersFromPersist(const std::map<std::string, double> &parameters) {
    auto parameterItems = parameterRegistry.getParameterItems();
    for (const auto &item : parameterItems) {
      auto it = parameters.find(item.paramKey);
      if (it != parameters.end()) {
        parameterTreeWrapper.setParameterValue(
            item.id, it->second, ptObserverToken,
            ParameterAutomationEventType::Value);
      }
    }
  }
};

} // namespace sonic