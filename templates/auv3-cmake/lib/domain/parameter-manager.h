#pragma once

#include "../core/parameter-definitions-provider.h"
#include "./parameters-store.h"
#include "interfaces.h"
#include <map>
#include <mutex>
#include <string>

namespace sonic {
class ParameterManager : public IParameterManager {
private:
  ParameterDefinitionsProvider &parameterDefinitionsProvider;
  ParametersStore parameterStore;

  mutable std::mutex parameterValuesMutex;

  std::map<int, std::function<void(std::string paramKey, double value)>>
      parameterChangeListeners;
  int nextToken = 0;

  void notifyParameterChange(const std::string &paramKey, double value) {
    for (auto &[id, listener] : parameterChangeListeners) {
      listener(paramKey, value);
    }
  }

public:
  ParameterManager(
      sonic::ParameterDefinitionsProvider &parameterDefinitionsProvider)
      : parameterDefinitionsProvider(parameterDefinitionsProvider) {
    auto maxId = 0;
    for (const auto &item : parameterDefinitionsProvider.getParameterItems()) {
      if (item.id > maxId) {
        maxId = item.id;
      }
    }
    parameterStore.setup(maxId);
    for (const auto &item : parameterDefinitionsProvider.getParameterItems()) {
      parameterStore.set(item.id, item.defaultValue);
    }
  }

  void setParameter(uint32_t paramId, double value, bool notifyToUi) override {
    std::scoped_lock lock(parameterValuesMutex);
    parameterStore.set(paramId, value);

    if (notifyToUi) {
      auto paramKey = parameterDefinitionsProvider.getParamKeyById(paramId);
      if (paramKey != std::nullopt) {
        notifyParameterChange(*paramKey, value);
      }
    }
  }

  double getParameter(uint32_t paramId) override {
    std::scoped_lock lock(parameterValuesMutex);
    return parameterStore.get(paramId);
  }

  int subscribeParameterChange(
      std::function<void(std::string paramKey, double value)> callback)
      override {
    auto id = nextToken++;
    parameterChangeListeners[id] = callback;
    return id;
  }

  void unsubscribeParameterChange(int subscriptionId) override {
    parameterChangeListeners.erase(subscriptionId);
  }

  void getAllParameters(std::map<std::string, double> &parameters) override {
    std::scoped_lock lock(parameterValuesMutex);
    for (const auto &item : parameterDefinitionsProvider.getParameterItems()) {
      auto paramId = item.id;
      auto paramKey = item.paramKey;
      auto value = parameterStore.get(paramId);
      parameters[paramKey] = value;
    }
  }
};
} // namespace sonic