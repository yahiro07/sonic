//------------------------------------------------------------------------
// Copyright(c) 2022 Steinberg Media Technologies GmbH.
//------------------------------------------------------------------------

#include "./project1_processor.h"
#include "./state_format.h"
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/base/ibstream.h"
#include "vst3wf/general/logger.h"
#include "vst3wf/logic/parameter_builder_impl.h"
#include "vst3wf/logic/parameter_item_helper.h"
#include "vst3wf/vst_entry/vst_entry_wrapper.h"
#include <base/source/fstreamer.h>
#include <cstring>
#include <glaze/glaze.hpp>
#include <glaze/json/write.hpp>
#include <pluginterfaces/vst/ivstevents.h>
#include <pluginterfaces/vst/ivstparameterchanges.h>
#include <unordered_map>

namespace Project1 {
using namespace Steinberg;

//------------------------------------------------------------------------
// Project1Processor
//------------------------------------------------------------------------
Project1Processor::Project1Processor() {
  //--- set the wanted controller for our processor
  setControllerClass(vst3wf::gPluginFactoryGlobalHolder.controllerCID);
  synthInstance = vst3wf::gPluginFactoryGlobalHolder.synthInstantiateFn();
}

//------------------------------------------------------------------------
Project1Processor::~Project1Processor() { delete synthInstance; }

float randF() { return (float)rand() / (float)RAND_MAX; }

//------------------------------------------------------------------------
tresult PLUGIN_API Project1Processor::initialize(FUnknown *context) {
  // Here the Plug-in will be instantiated
  printf("HelloWorldProcessor::initialize\n");

  //---always initialize the parent-------
  tresult result = AudioEffect::initialize(context);
  // if everything Ok, continue
  if (result != kResultOk) {
    return result;
  }

  //--- create Audio IO ------
  addAudioInput(STR16("Stereo In"), Steinberg::Vst::SpeakerArr::kStereo);
  addAudioOutput(STR16("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo);

  /* If you don't need an event bus, you can remove the next line */
  addEventInput(STR16("Event In"), 1);

  getAudioOutput(0)->setFlags(Vst::BusInfo::kDefaultActive);

  auto parameterBuilder = Amx::ParameterBuilderImpl();
  synthInstance->setupParameters(parameterBuilder);
  auto parameterItems = parameterBuilder.getItems();
  parameterDefinitionsProvider.addParameters(parameterItems);
  for (auto &item : parameterItems) {
    parametersCache[item.address] = item.defaultValue;
  }

  return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Project1Processor::terminate() {
  // Here the Plug-in will be de-instantiated, last possibility to remove some
  // memory!

  //---do not forget to call parent ------
  return AudioEffect::terminate();
}

//------------------------------------------------------------------------
tresult PLUGIN_API Project1Processor::setActive(TBool state) {
  printf("Project1Processor::setActive %d\n", state);
  //--- called when the Plug-in is enable/disable (On/Off) -----
  return AudioEffect::setActive(state);
}

//------------------------------------------------------------------------
tresult PLUGIN_API Project1Processor::process(Vst::ProcessData &data) {
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
              Amx::ParameterItemHelper::getUnnormalized(paramItem, value);
          // vst3wf::logger.log("parameter %d received in audio thread %f %f",
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
          vst3wf::logger.log("note on %d", event.noteOn.pitch);
        } else if (event.type == Vst::Event::kNoteOffEvent) {
          synthInstance->noteOff(event.noteOff.pitch);
          vst3wf::logger.log("note off %d", event.noteOff.pitch);
        }
      }
    }
  }

  //--- Process Audio---------------------
  //--- ----------------------------------
  if (
      // data.numInputs == 0 ||
      data.numOutputs == 0) {
    // nothing to do
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

    if (outL && outR) {
      synthInstance->process(outL, outR, data.numSamples);
    } else if (outL) {
      synthInstance->process(outL, outL, data.numSamples);
    }
  }
  return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API
Project1Processor::setupProcessing(Vst::ProcessSetup &newSetup) {
  printf("setupProcessing sampleRate: %f, maxSamplesPerBlock: %d\n",
         newSetup.sampleRate, newSetup.maxSamplesPerBlock);
  synthInstance->prepare(newSetup.sampleRate, newSetup.maxSamplesPerBlock);

  //--- called before any processing ----
  return AudioEffect::setupProcessing(newSetup);
}

//------------------------------------------------------------------------
tresult PLUGIN_API
Project1Processor::canProcessSampleSize(int32 symbolicSampleSize) {
  // by default kSample32 is supported
  if (symbolicSampleSize == Vst::kSample32)
    return kResultTrue;

  // disable the following comment if your processing support kSample64
  /* if (symbolicSampleSize == Vst::kSample64)
          return kResultTrue; */

  return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Project1Processor::getState(IBStream *state) {
  // here we need to save the model (preset or project)

  vst3wf::logger.log("Project1Processor::getState");
  if (!state)
    return kResultFalse;

  constexpr int32 kStateMagic = 0x534F4E43; // 'S''O''N''C'
  constexpr int32 kStateVersion = 1;

  // Version for *parameter schema* (migration target).
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

  std::string jsonStr{};
  auto ec = glz::write_jsonc(processorState, jsonStr);
  if (ec) {
    vst3wf::logger.log("error writing json: %s",
                       glz::format_error(ec, jsonStr).c_str());
    return kResultFalse;
  }
  vst3wf::logger.log("jsonStr: %s", jsonStr.c_str());

  IBStreamer streamer(state, kLittleEndian);
  streamer.writeInt32(kStateMagic);
  streamer.writeInt32(kStateVersion);
  streamer.writeInt32(static_cast<int32>(jsonStr.size()));
  if (!jsonStr.empty()) {
    const auto bytesWritten =
        streamer.writeRaw(jsonStr.data(), static_cast<int32>(jsonStr.size()));
    if (bytesWritten != static_cast<int32>(jsonStr.size())) {
      vst3wf::logger.log("failed writing state bytes: %d/%d",
                         static_cast<int32>(bytesWritten),
                         static_cast<int32>(jsonStr.size()));
      return kResultFalse;
    }
  }

  return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Project1Processor::setState(IBStream *state) {
  // called when we load a preset or project, the model has to be reloaded

  vst3wf::logger.log("Project1Processor::setState");
  if (!state)
    return kResultFalse;

  constexpr int32 kStateMagic = 0x534F4E43; // 'S''O''N''C'
  constexpr int32 kMaxStateBytes = 1024 * 1024;

  IBStreamer streamer(state, kLittleEndian);
  int32 firstWord = 0;
  if (!streamer.readInt32(firstWord))
    return kResultFalse;
  if (firstWord != kStateMagic)
    return kResultFalse;
  int32 stateVersion = 0;
  int32 size = 0;
  if (!streamer.readInt32(stateVersion))
    return kResultFalse;
  if (!streamer.readInt32(size))
    return kResultFalse;

  if (size < 0 || size > kMaxStateBytes) {
    vst3wf::logger.log("invalid state size: %d", size);
    return kResultFalse;
  }

  std::string jsonStr;
  jsonStr.resize(static_cast<size_t>(size));
  if (size > 0) {
    const auto bytesRead = streamer.readRaw(jsonStr.data(), size);
    if (bytesRead != size) {
      vst3wf::logger.log("failed reading state bytes: %d/%d",
                         static_cast<int32>(bytesRead), size);
      return kResultFalse;
    }
  } else {
    // Accept empty state (keep defaults)
    vst3wf::logger.log("empty state (size=0)");
    return kResultOk;
  }
  ProcessorState processorState{};
  processorState.parametersVersion = 0;

  // format: {"parametersVersion": N, "parameters": {"id": v, ...}}
  auto ec = glz::read_jsonc(processorState, jsonStr);
  if (ec) {
    vst3wf::logger.log("error reading json: %s",
                       glz::format_error(ec, jsonStr).c_str());
    return kResultFalse;
  }
  vst3wf::logger.log("jsonStr: %s", jsonStr.c_str());

  if (stateVersion != 0) {
    vst3wf::logger.log("state version: %d", stateVersion);
  }

  vst3wf::logger.log("parametersVersion: %d", processorState.parametersVersion);

  for (auto &kv : processorState.parameters) {
    auto paramItem =
        parameterDefinitionsProvider.getParameterItemByIdentifier(kv.first);
    if (!paramItem) {
      vst3wf::logger.log("unknown parameter identifier in state: %s",
                         kv.first.c_str());
      continue;
    }
    parametersCache[paramItem->address] = kv.second;
    synthInstance->setParameter(paramItem->address, kv.second);
  }

  return kResultOk;
}

//------------------------------------------------------------------------
} // namespace Project1
