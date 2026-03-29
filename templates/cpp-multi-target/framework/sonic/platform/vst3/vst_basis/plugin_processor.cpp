#include "./plugin_processor.h"
#include "../logic/parameters-initializer.h"
#include "../support/processor_state_helper.h"
#include "sonic/core/persistence.h"
#include <pluginterfaces/vst/ivstevents.h>
#include <pluginterfaces/vst/ivstparameterchanges.h>
#include <sonic/common/logger.h>
#include <sonic/core/parameter-spec-helper.h>

namespace vst_basis {

tresult PLUGIN_API PluginProcessor::initialize(FUnknown *context) {
  tresult result = AudioEffect::initialize(context);
  if (result != kResultOk) {
    return result;
  }
  addAudioInput(STR16("Stereo In"), Vst::SpeakerArr::kStereo);
  addAudioOutput(STR16("Stereo Out"), Vst::SpeakerArr::kStereo);

  addEventInput(STR16("Event In"), 1);

  getAudioOutput(0)->setFlags(Vst::BusInfo::kDefaultActive);

  initializeParameters(*synthInstance, parameterRegistry, parameterStore);

  return kResultOk;
}

tresult PLUGIN_API PluginProcessor::terminate() {
  return AudioEffect::terminate();
}

tresult PLUGIN_API PluginProcessor::setActive(TBool state) {
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
          parameterStore.set(paramId, unnormalizedValue);
          synthInstance->setParameter(paramId, unnormalizedValue);
        }
      }
    }
  }

  //---Read upstream events-------------
  UpstreamEvent e;
  while (upstreamEventQueue.pop(e)) {
    if (e.type == UpstreamEventType::NoteRequest) {
      // note requested from ui
      if (e.note.velocity > 0.0) {
        synthInstance->noteOn(e.note.noteNumber, e.note.velocity);
      } else {
        synthInstance->noteOff(e.note.noteNumber);
      }
      // return response
      downstreamEventQueue.push({
          .type = DownstreamEventType::HostNote,
          .note = {e.note.noteNumber, e.note.velocity},
      });
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
          downstreamEventQueue.push({
              .type = DownstreamEventType::HostNote,
              .note{.noteNumber = event.noteOn.noteId,
                    .velocity = event.noteOn.velocity},
          });
        } else if (event.type == Vst::Event::kNoteOffEvent) {
          synthInstance->noteOff(event.noteOff.pitch);
          downstreamEventQueue.push({
              .type = DownstreamEventType::HostNote,
              .note{.noteNumber = event.noteOff.noteId, .velocity = 0.0},
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

tresult PLUGIN_API PluginProcessor::getState(IBStream *stream) {
  if (!stream)
    return kResultFalse;
  PersistStateData data;
  for (auto item : parameterRegistry.getParameterItems()) {
    auto value = parameterStore.get(item.id);
    data.parameters[item.paramKey] = value;
  }
  processorStateHelper_writeState(stream, data);
  return kResultOk;
}

tresult PLUGIN_API PluginProcessor::setState(IBStream *stream) {
  if (!stream)
    return kResultFalse;

  PersistStateData data;
  auto ok = processorStateHelper_readState(stream, data);
  if (!ok) {
    return kResultFalse;
  }
  for (auto &kv : data.parameters) {
    auto paramItem = parameterRegistry.getParameterItemByParamKey(kv.first);
    if (!paramItem)
      continue;
    parameterStore.set(paramItem->id, kv.second);
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
