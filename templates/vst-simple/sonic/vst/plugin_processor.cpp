#include "./plugin_processor.h"
#include "../general/logger.h"
#include "../logic/parameter_builder_impl.h"
#include "../logic/parameter_item_helper.h"
#include "../modules/processor_state_helper.h"
#include "../vst_entry/vst_entry_wrapper.h"
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstevents.h"
#include <base/source/fstreamer.h>
#include <cstring>
#include <glaze/glaze.hpp>
#include <glaze/json/write.hpp>
#include <pluginterfaces/vst/ivstevents.h>
#include <pluginterfaces/vst/ivstparameterchanges.h>
#include <unordered_map>

namespace vst3wf_plugin {

PluginProcessor::PluginProcessor() : processorSideMessagingBridge(*this) {
  setControllerClass(gPluginFactoryGlobalHolder.controllerCID);
  synthInstance = gPluginFactoryGlobalHolder.synthInstantiateFn();
}

PluginProcessor::~PluginProcessor() { delete synthInstance; }

float randF() { return (float)rand() / (float)RAND_MAX; }
tresult PLUGIN_API PluginProcessor::initialize(FUnknown *context) {
  printf("PluginProcessor::initialize\n");
  tresult result = AudioEffect::initialize(context);
  if (result != kResultOk) {
    return result;
  }
  addAudioInput(STR16("Stereo In"), Vst::SpeakerArr::kStereo);
  addAudioOutput(STR16("Stereo Out"), Vst::SpeakerArr::kStereo);

  addEventInput(STR16("Event In"), 1);

  getAudioOutput(0)->setFlags(Vst::BusInfo::kDefaultActive);

  auto parameterBuilder = ParameterBuilderImpl();
  synthInstance->setupParameters(parameterBuilder);
  auto parameterItems = parameterBuilder.getItems();
  parameterDefinitionsProvider.addParameters(parameterItems);
  for (auto &item : parameterItems) {
    parametersCache[item.address] = item.defaultValue;
  }

  return kResultOk;
}

tresult PLUGIN_API PluginProcessor::terminate() {
  return AudioEffect::terminate();
}

tresult PLUGIN_API PluginProcessor::setActive(TBool state) {
  printf("PluginProcessor::setActive %d\n", state);
  return AudioEffect::setActive(state);
}

tresult PLUGIN_API PluginProcessor::process(Vst::ProcessData &data) {
  //--- Read inputs parameter changes-----------
  if (data.inputParameterChanges) {
    int32 numParamsChanged = data.inputParameterChanges->getParameterCount();
    for (int32 index = 0; index < numParamsChanged; index++) {
      Vst::IParamValueQueue *paramQueue =
          data.inputParameterChanges->getParameterData(index);
      if (paramQueue) {
        Vst::ParamValue value;
        int32 sampleOffset;
        int32 numPoints = paramQueue->getPointCount();

        auto paramId = paramQueue->getParameterId();
        if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) ==
            kResultTrue) {
          auto paramItem =
              parameterDefinitionsProvider.getParameterItemByAddress(paramId);
          const auto unnormalizedValue =
              ParameterItemHelper::getUnnormalized(paramItem, value);
          // logger.log("parameter %d received in audio thread %f %f",
          //                    paramId, value, unnormalizedValue);
          parametersCache[paramId] = unnormalizedValue;
          synthInstance->setParameter(paramId, unnormalizedValue);
        }
      }
    }
  }

  //---Read input events-------------
  if (Vst::IEventList *eventList = data.inputEvents) {
    int32 numEvent = eventList->getEventCount();
    for (int32 i = 0; i < numEvent; i++) {
      Vst::Event event{};
      if (eventList->getEvent(i, event) == kResultOk) {
        if (event.type == Vst::Event::kNoteOnEvent) {
          synthInstance->noteOn(event.noteOn.pitch, event.noteOn.velocity);
          // logger.log("note on %d", event.noteOn.pitch);
          realtimeHostEventQueue.push({
              .type = RealtimeHostEventType::NoteOn,
              .data1 = event.noteOn.pitch,
              .data2 = event.noteOn.velocity,
          });
        } else if (event.type == Vst::Event::kNoteOffEvent) {
          synthInstance->noteOff(event.noteOff.pitch);
          // logger.log("note off %d", event.noteOff.pitch);
          realtimeHostEventQueue.push({
              .type = RealtimeHostEventType::NoteOff,
              .data1 = event.noteOff.pitch,
              .data2 = 0,
          });
        }
      }
    }
  }

  if (data.numOutputs == 0) {
    return kResultOk;
  }

  // This example implementation writes only 32-bit float samples.
  if (data.symbolicSampleSize != Vst::kSample32)
    return kResultOk;

  auto *outL = data.outputs[0].channelBuffers32[0];
  auto *outR = (data.outputs[0].numChannels > 1)
                   ? data.outputs[0].channelBuffers32[1]
                   : nullptr;

  if (data.numSamples > 0 && outL) {
    // data.outputs[0].silenceFlags = 0;

    memset(outL, 0, sizeof(float) * data.numSamples);
    if (outR) {
      memset(outR, 0, sizeof(float) * data.numSamples);
    }

    if (outL && outR) {
      synthInstance->process(outL, outR, data.numSamples);
    } else if (outL) {
      synthInstance->process(outL, outL, data.numSamples);
    }
  }
  return kResultOk;
}

tresult PLUGIN_API
PluginProcessor::setupProcessing(Vst::ProcessSetup &newSetup) {
  printf("setupProcessing sampleRate: %f, maxSamplesPerBlock: %d\n",
         newSetup.sampleRate, newSetup.maxSamplesPerBlock);
  synthInstance->prepare(newSetup.sampleRate, newSetup.maxSamplesPerBlock);
  return AudioEffect::setupProcessing(newSetup);
}

tresult PLUGIN_API
PluginProcessor::canProcessSampleSize(int32 symbolicSampleSize) {
  // by default kSample32 is supported
  if (symbolicSampleSize == Vst::kSample32)
    return kResultTrue;

  // disable the following comment if your processing support kSample64
  /* if (symbolicSampleSize == Vst::kSample64)
          return kResultTrue; */

  return kResultFalse;
}

tresult PLUGIN_API PluginProcessor::getState(IBStream *state) {
  logger.log("PluginProcessor::getState");
  if (!state)
    return kResultFalse;

  constexpr int kParametersVersion = 1;
  ProcessorState processorState{};
  processorState.parametersVersion = kParametersVersion;
  for (auto &kv : parametersCache) {
    auto paramItem =
        parameterDefinitionsProvider.getParameterItemByAddress(kv.first);
    if (!paramItem)
      continue;
    processorState.parameters[paramItem->identifier] = kv.second;
  }

  processorStateHelper_writeState(state, processorState);
  return kResultOk;
}

tresult PLUGIN_API PluginProcessor::setState(IBStream *state) {
  if (!state)
    return kResultFalse;

  ProcessorState processorState;
  auto ok = processorStateHelper_readState(state, processorState);
  if (!ok) {
    return kResultFalse;
  }
  for (auto &kv : processorState.parameters) {
    auto paramItem =
        parameterDefinitionsProvider.getParameterItemByIdentifier(kv.first);
    if (!paramItem)
      continue;
    parametersCache[paramItem->address] = kv.second;
    synthInstance->setParameter(paramItem->address, kv.second);
  }

  return kResultOk;
}

tresult PLUGIN_API PluginProcessor::notify(Vst::IMessage *message) {
  auto wm = processorSideMessagingBridge.decodeMessage(message);
  if (!wm.has_value()) {
    return AudioEffect::notify(message);
  }

  if (wm->type == WrappedMessageType::noteOnRequestFromEditor) {
    auto noteNumber = wm->noteOnRequestFromEditor.noteNumber;
    auto velocity = wm->noteOnRequestFromEditor.velocity;
    synthInstance->noteOn(noteNumber, velocity);
    processorSideMessagingBridge.sendHostNoteOn(noteNumber, velocity);
  } else if (wm->type == WrappedMessageType::noteOffRequestFromEditor) {
    auto noteNumber = wm->noteOffRequestFromEditor.noteNumber;
    synthInstance->noteOff(noteNumber);
    processorSideMessagingBridge.sendHostNoteOff(noteNumber);
  } else if (wm->type == WrappedMessageType::pullProcessorSideEvents) {
    RealtimeHostEvent e;
    int count = 0;
    while (realtimeHostEventQueue.pop(e) && count < 64) {
      if (e.type == RealtimeHostEventType::NoteOn) {
        processorSideMessagingBridge.sendHostNoteOn(e.data1, e.data2);
      } else if (e.type == RealtimeHostEventType::NoteOff) {
        processorSideMessagingBridge.sendHostNoteOff(e.data1);
      }
      count++;
    }
  }
  return kResultOk;
}

} // namespace vst3wf_plugin
