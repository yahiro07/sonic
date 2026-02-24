//------------------------------------------------------------------------
// Copyright(c) 2022 Steinberg Media Technologies GmbH.
//------------------------------------------------------------------------

#include "project1_processor.h"
#include "base/source/fstreamer.h"
#include "dsp/MySynthesizer.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "project1_cids.h"

#include "utils/logger.h"
#include <cstring>

namespace Project1 {
using namespace Steinberg;

//------------------------------------------------------------------------
// Project1Processor
//------------------------------------------------------------------------
Project1Processor::Project1Processor() {
  //--- set the wanted controller for our processor
  setControllerClass(kProject1ControllerUID);
  synthInstance = createSynthesizerInstance();
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
          synthInstance->setParameter(paramId, value);
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
          logger.log("note on %d", event.noteOn.pitch);
        } else if (event.type == Vst::Event::kNoteOffEvent) {
          synthInstance->noteOff(event.noteOff.pitch);
          logger.log("note off %d", event.noteOff.pitch);
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
tresult PLUGIN_API Project1Processor::setState(IBStream *state) {
  if (!state)
    return kResultFalse;

  // called when we load a preset or project, the model has to be reloaded

  // IBStreamer streamer(state, kLittleEndian);

  // float savedParam1 = 0.f;
  // if (streamer.readFloat(savedParam1) == false)
  //   return kResultFalse;

  // int32 savedParam2 = 0;
  // if (streamer.readInt32(savedParam2) == false)
  //   return kResultFalse;

  // int32 savedBypass = 0;
  // if (streamer.readInt32(savedBypass) == false)
  //   return kResultFalse;

  // mParam1 = savedParam1;
  // mParam2 = savedParam2 > 0 ? 1 : 0;
  // mBypass = savedBypass > 0;

  return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Project1Processor::getState(IBStream *state) {
  // here we need to save the model (preset or project)

  // float toSaveParam1 = mParam1;
  // int32 toSaveParam2 = mParam2;
  // int32 toSaveBypass = mBypass ? 1 : 0;

  // IBStreamer streamer(state, kLittleEndian);
  // streamer.writeFloat(toSaveParam1);
  // streamer.writeInt32(toSaveParam2);
  // streamer.writeInt32(toSaveBypass);

  return kResultOk;
}

//------------------------------------------------------------------------
} // namespace Project1
