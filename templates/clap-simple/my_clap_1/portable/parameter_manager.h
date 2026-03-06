#pragma once

#include "interfaces.h"
#include "sonic_common/logic/parameter_builder_impl.h"
#include "sonic_common/logic/parameter_definitions_provider.h"
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>

class ParameterManager : public IParameterManager {
private:
  sonic_common::ParameterDefinitionsProvider &parameterDefinitionsProvider;

  std::unordered_map<uint32_t, double> parameterValues;
  mutable std::mutex parameterValuesMutex;

  std::map<int, std::function<void(std::string identifier, double value)>>
      parameterChangeListeners;

  void notifyParameterChange(const std::string &identifier, double value) {
    for (auto &[id, listener] : parameterChangeListeners) {
      listener(identifier, value);
    }
  }

public:
  ParameterManager(
      sonic_common::ParameterDefinitionsProvider &parameterDefinitionsProvider)
      : parameterDefinitionsProvider(parameterDefinitionsProvider) {}

  void setupParameters(IPluginSynthesizer &synth, uint64_t maxAddress) {
    auto parameterBuilder = sonic_common::ParameterBuilderImpl();
    synth.setupParameters(parameterBuilder);
    auto parameterItems = parameterBuilder.getItems();
    parameterDefinitionsProvider.addParameters(parameterItems, maxAddress);
    for (auto &item : parameterItems) {
      synth.setParameter(item.address, item.defaultValue);
      this->setParameter(item.address, item.defaultValue, false);
    }
  }

  void setParameter(uint32_t paramId, double value, bool notifyToUi) {
    std::scoped_lock lock(parameterValuesMutex);
    parameterValues[paramId] = value;

    if (notifyToUi) {
      auto identifier =
          parameterDefinitionsProvider.getIdentifierByAddress(paramId);
      if (identifier != std::nullopt) {
        notifyParameterChange(*identifier, value);
      }
    }
  }

  double getParameter(uint32_t paramId) const {
    std::scoped_lock lock(parameterValuesMutex);
    auto it = parameterValues.find(paramId);
    if (it == parameterValues.end()) {
      return 0.0;
    }
    return it->second;
  }

  int subscribeParameterChange(
      std::function<void(std::string identifier, double value)> callback)
      override {
    auto id = parameterChangeListeners.size() + 1;
    parameterChangeListeners[id] = callback;
    return id;
  }

  void unsubscribeParameterChange(int subscriptionId) override {
    parameterChangeListeners.erase(subscriptionId);
  }

  void getAllParameters(std::map<std::string, double> &parameters) override {
    std::scoped_lock lock(parameterValuesMutex);
    for (const auto item : parameterDefinitionsProvider.getParameterItems()) {
      auto paramId = item.address;
      auto identifier = item.identifier;
      auto value = parameterValues[paramId];
      parameters[identifier] = value;
    }
  }
};