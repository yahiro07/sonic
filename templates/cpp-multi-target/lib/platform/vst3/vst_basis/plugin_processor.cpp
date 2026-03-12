#include "./plugin_processor.h"
#include "../../../common/logger.h"
#include "../../../core/parameter-spec-helper.h"
#include "../logic/parameters-initializer.h"
#include "../modules/processor_state_helper.h"
#include <pluginterfaces/vst/ivstevents.h>
#include <pluginterfaces/vst/ivstparameterchanges.h>

namespace vst_basis {

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

  initializeParameters(*synthInstance, parameterRegistry, parametersStore);

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
          auto paramItem = parameterRegistry.getParameterItemById(paramId);
          const auto unnormalizedValue =
              ParameterSpecHelper::getUnnormalized(paramItem, value);
          // logger.log("parameter %d received in audio thread %f %f",
          //                    paramId, value, unnormalizedValue);
          parametersStore.set(paramId, unnormalizedValue);
          synthInstance->setParameter(paramId, unnormalizedValue);
        }
      }
    }
  }

  //---Read upstream events-------------
  UpstreamEvent e;
  while (upstreamEventQueue.pop(e)) {
    if (e.type == UpstreamEventType::NoteRequest) {
      if (e.note.velocity > 0.0) {
        synthInstance->noteOn(e.note.noteNumber, e.note.velocity);
      } else {
        synthInstance->noteOff(e.note.noteNumber);
      }
    }
  }

  //---Read input events-------------
  if (Vst::IEventList *eventList = data.inputEvents) {
    int32 numEvent = eventList->getEventCount();
    for (int32 i = 0; i < numEvent; i++) {
      Vst::Event event{};
      if (eventList->getEvent(i, event) == kResultOk) {
        if (event.type == Vst::Event::kNoteOnEvent ||
            event.type == Vst::Event::kNoteOffEvent) {
          synthInstance->noteOn(event.noteOn.pitch, event.noteOn.velocity);
          // logger.log("note on %d", event.noteOn.pitch);
          auto noteNumber = event.noteOn.pitch;
          auto velocity = event.noteOn.velocity;
          downstreamEventQueue.push({
              .type = DownstreamEventType::HostNote,
              .note{noteNumber, velocity},
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
      synthInstance->processAudio(outL, outR, data.numSamples);
    } else if (outL) {
      synthInstance->processAudio(outL, outL, data.numSamples);
    }
  }
  return kResultOk;
}

tresult PLUGIN_API
PluginProcessor::setupProcessing(Vst::ProcessSetup &newSetup) {
  printf("setupProcessing sampleRate: %f, maxSamplesPerBlock: %d\n",
         newSetup.sampleRate, newSetup.maxSamplesPerBlock);
  synthInstance->prepareProcessing(newSetup.sampleRate,
                                   newSetup.maxSamplesPerBlock);
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
  for (auto item : parameterRegistry.getParameterItems()) {
    auto value = parametersStore.get(item.id);
    processorState.parameters[item.paramKey] = value;
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
    auto paramItem = parameterRegistry.getParameterItemByParamKey(kv.first);
    if (!paramItem)
      continue;
    parametersStore.set(paramItem->id, kv.second);
    synthInstance->setParameter(paramItem->id, kv.second);
  }

  return kResultOk;
}

tresult PLUGIN_API PluginProcessor::notify(Vst::IMessage *message) {
  UpstreamEvent e;
  if (processorSideMessagePort.decodeMessage(message, e)) {
    if (e.type == UpstreamEventType::NoteRequest) {
      upstreamEventQueue.push(UpstreamEvent{
          .type = UpstreamEventType::NoteRequest, .note = e.note});
    } else if (e.type == UpstreamEventType::PollingProcessorSideEvent) {
      DownstreamEvent event;
      while (downstreamEventQueue.pop(event)) {
        processorSideMessagePort.sendDownstreamEvent(event);
      }
    }
    return kResultOk;
  }
  return AudioEffect::notify(message);
}

} // namespace vst_basis
