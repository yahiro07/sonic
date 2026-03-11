#pragma once
#include "../api/synthesizer-base.h"
#include "../common/spsc-queue.h"
#include "../core/parameter-builder-impl.h"
#include "../core/parameter-definitions-provider.h"
#include "../domain/interfaces.h"
#include "../domain/parameters-store.h"
#include "./controller-facade.h"
#include "./controller-parameter-port.h"
#include "./events.h"
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
  ParameterDefinitionsProvider parametersDefinitionProvider;
  SynthesizerBase &synth;
  ParameterTreeWrapper &parameterTreeWrapper;
  ControllerParameterPort controllerParameterPort;
  ControllerFacade controllerFacade;
  ParametersStore parametersStore;

  SPSCQueue<UpstreamEvent, 32> upstreamEventQueue;
  SPSCQueue<DownstreamEvent, 32> downstreamEventQueue;

public:
  static std::vector<sonic::ParameterItem>
  preGenerateParameterItems(SynthesizerBase &synth) {
    ParameterBuilderImpl builder;
    synth.setupParameters(builder);
    return builder.getItems();
  }
  EntryController(SynthesizerBase &synth,
                  std::vector<ParameterItem> &parameteritems,
                  ParameterTreeWrapper &parameterTreeWrapper)
      : synth(synth), parameterTreeWrapper(parameterTreeWrapper),
        controllerParameterPort(parameterTreeWrapper,
                                parametersDefinitionProvider),
        controllerFacade(controllerParameterPort) {
    parametersDefinitionProvider.addParameters(parameteritems, 0xFFFFFFFF);
  }

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

    controllerFacade.noteRequestedPort.subscribe(
        [this](int noteNumber, float velocity) {
          upstreamEventQueue.push(UpstreamEvent{
              .type = UpstreamEventType::NoteRequest,
              .note = {noteNumber, velocity},
          });
        });
  }

  bool popUpstreamEvent(UpstreamEvent &e) { return upstreamEventQueue.pop(e); }

  void terminate() { controllerFacade.noteRequestedPort.unsubscribe(); }

  IControllerFacade &getControllerFacade() { return controllerFacade; }
};
}; // namespace sonic
