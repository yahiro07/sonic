#pragma once
#include "../api/synthesizer-base.h"
#include "../common/spsc-queue.h"
#include "../core/parameter-builder-impl.h"
#include "../core/parameter-registry.h"
#include "../domain/interfaces.h"
#include "../domain/parameters-store.h"
#include "./controller-facade.h"
#include "./events.h"
#include "./parameters-service.h"
#include "logic/note-service.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

namespace sonic {

static int getMaxIdFromParameterItems(const ParameterSpecArray &items) {
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
  ParameterRegistry parametersRegistry;
  SynthesizerBase &synth;
  ParameterTreeWrapper &parameterTreeWrapper;
  ParameterService parameterService;
  NoteService noteService;
  ControllerFacade controllerFacade;
  ParametersStore parametersStore;

  SPSCQueue<UpstreamEvent, 32> upstreamEventQueue;
  SPSCQueue<DownstreamEvent, 32> downstreamEventQueue;

public:
  static ParameterSpecArray preGenerateParameterItems(SynthesizerBase &synth) {
    ParameterBuilderImpl builder;
    synth.setupParameters(builder);
    return builder.getItems();
  }
  EntryController(SynthesizerBase &synth, ParameterSpecArray &parameteritems,
                  ParameterTreeWrapper &parameterTreeWrapper)
      : synth(synth), parameterTreeWrapper(parameterTreeWrapper),
        parameterService(parameterTreeWrapper, parametersRegistry),
        controllerFacade(parameterService, noteService) {
    parametersRegistry.addParameters(parameteritems, 0xFFFFFFFF);
  }

  void initialize() {
    auto parameterItems = parametersRegistry.getParameterItems();
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

    noteService.noteRequestPort.subscribe(
        [this](int noteNumber, float velocity) {
          // requested note from ui
          // send note request to audio thread via queue
          upstreamEventQueue.push(UpstreamEvent{
              .type = UpstreamEventType::NoteRequest,
              .note = {noteNumber, velocity},
          });
          // return host note event to show active notes in ui
          noteService.hostNotePort.call(noteNumber, velocity);
        });
  }

  void terminate() { noteService.noteRequestPort.unsubscribe(); }

  bool popUpstreamEvent(UpstreamEvent &e) { return upstreamEventQueue.pop(e); }

  void pushDownstreamEvent(const DownstreamEvent &e) {
    downstreamEventQueue.push(e);
  }

  void onTimer() {
    DownstreamEvent e;
    while (downstreamEventQueue.pop(e)) {
      if (e.type == DownstreamEventType::HostNote) {
        noteService.hostNotePort.call(e.note.noteNumber, e.note.velocity);
      }
    }
  }

  IControllerFacade &getControllerFacade() { return controllerFacade; }
};
}; // namespace sonic
