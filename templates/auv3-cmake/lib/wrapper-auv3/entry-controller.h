#pragma once
#include "../api/synthesizer-base.h"
#include "../core/parameter-builder-impl.h"
#include "../core/parameter-definitions-provider.h"
#include "../domain/interfaces.h"
#include "../domain/parameters-store.h"
#include "./controller-facade.h"
#include "./controller-parameter-port.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

namespace sonic {

static int getMaxIdFromParameterItems(const std::vector<ParameterItem> &items) {
  int maxId = 0;
  for (const auto &item : items) {
    if (item.id > maxId) {
      maxId = item.id;
    }
  }
  return maxId;
}

class EntryController {
private:
  SynthesizerBase &synth;
  ParameterDefinitionsProvider &parametersDefinitionProvider;
  ParameterTreeWrapper &parameterTreeWrapper;
  ControllerParameterPort controllerParameterPort;
  ControllerFacade controllerFacade;
  ParametersStore parametersStore;

public:
  static std::vector<sonic::ParameterItem>
  preGenerateParameterItems(SynthesizerBase &synth) {
    ParameterBuilderImpl builder;
    synth.setupParameters(builder);
    return builder.getItems();
  }
  EntryController(SynthesizerBase &synth,
                  ParameterDefinitionsProvider &parametersDefinitionProvider,
                  ParameterTreeWrapper &parameterTreeWrapper)
      : synth(synth),
        parametersDefinitionProvider(parametersDefinitionProvider),
        parameterTreeWrapper(parameterTreeWrapper),
        controllerParameterPort(parameterTreeWrapper,
                                parametersDefinitionProvider),
        controllerFacade(controllerParameterPort) {}

  void initialize() {
    auto parameterItems = parametersDefinitionProvider.getParameterItems();
    auto maxId = getMaxIdFromParameterItems(parameterItems);
    parametersStore.setup(maxId);
    for (const auto &item : parameterItems) {
      parametersStore.set(item.id, item.defaultValue);
    }
    parameterTreeWrapper.setImplementorValueObserver(
        [this](uint64_t address, float value) {
          auto id = (int32_t)address;
          this->parametersStore.set(id, value);
          this->synth.setParameter(id, value);
        });
    parameterTreeWrapper.setImplementorValueProvider([this](uint64_t address) {
      auto id = (int32_t)address;
      return parametersStore.get(id);
    });
  }

  IControllerFacade &getControllerFacade() { return controllerFacade; }
};
}; // namespace sonic
