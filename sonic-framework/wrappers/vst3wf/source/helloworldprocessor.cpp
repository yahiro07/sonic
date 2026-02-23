//------------------------------------------------------------------------
// Copyright(c) 2022 Steinberg Media Technologies GmbH.
//------------------------------------------------------------------------

#include "helloworldprocessor.h"
#include "helloworldcids.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

#include <cstring>

using namespace Steinberg;

namespace Steinberg {
//------------------------------------------------------------------------
// HelloWorldProcessor
//------------------------------------------------------------------------
HelloWorldProcessor::HelloWorldProcessor() {
  //--- set the wanted controller for our processor
  setControllerClass(kHelloWorldControllerUID);
}

//------------------------------------------------------------------------
HelloWorldProcessor::~HelloWorldProcessor() {}

float randF() { return (float)rand() / (float)RAND_MAX; }

//------------------------------------------------------------------------
tresult PLUGIN_API HelloWorldProcessor::initialize(FUnknown *context) {
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
tresult PLUGIN_API HelloWorldProcessor::terminate() {
  // Here the Plug-in will be de-instantiated, last possibility to remove some
  // memory!

  //---do not forget to call parent ------
  return AudioEffect::terminate();
}

//------------------------------------------------------------------------
tresult PLUGIN_API HelloWorldProcessor::setActive(TBool state) {
  printf("HelloWorldProcessor::setActive %d\n", state);
  //--- called when the Plug-in is enable/disable (On/Off) -----
  return AudioEffect::setActive(state);
}

//------------------------------------------------------------------------
tresult PLUGIN_API HelloWorldProcessor::process(Vst::ProcessData &data) {
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
        switch (paramQueue->getParameterId()) {
        case HelloWorldParams::kParamVolId:
          if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) ==
              kResultTrue)
            mParam1 = value;
          break;
        case HelloWorldParams::kParamOnId:
          if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) ==
              kResultTrue)
            mParam2 = value > 0 ? 1 : 0;
          break;
        case HelloWorldParams::kBypassId:
          if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) ==
              kResultTrue)
            mBypass = (value > 0.5f);
          break;
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

  const bool generate = (mParam2 != 0) && (mBypass == false);
  if (data.numSamples > 0 && outL) {
    if (generate) {
      data.outputs[0].silenceFlags = 0;

      const float volume = static_cast<float>(mParam1);
      for (int32 i = 0; i < data.numSamples; i++) {
        const float y = ((randF() * 2.f) - 1.f) * volume;
        outL[i] = y;
        if (outR)
          outR[i] = y;
      }
    } else {
      // clear output (and tell the host it's silent)
      std::memset(outL, 0,
                  static_cast<size_t>(data.numSamples) * sizeof(float));
      if (outR)
        std::memset(outR, 0,
                    static_cast<size_t>(data.numSamples) * sizeof(float));

      const uint64 silenceMask =
          (data.outputs[0].numChannels >= 64)
              ? ~uint64(0)
              : ((uint64(1) << data.outputs[0].numChannels) - uint64(1));
      data.outputs[0].silenceFlags = silenceMask;
    }
  }
  return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API
HelloWorldProcessor::setupProcessing(Vst::ProcessSetup &newSetup) {
  //--- called before any processing ----
  return AudioEffect::setupProcessing(newSetup);
}

//------------------------------------------------------------------------
tresult PLUGIN_API
HelloWorldProcessor::canProcessSampleSize(int32 symbolicSampleSize) {
  // by default kSample32 is supported
  if (symbolicSampleSize == Vst::kSample32)
    return kResultTrue;

  // disable the following comment if your processing support kSample64
  /* if (symbolicSampleSize == Vst::kSample64)
          return kResultTrue; */

  return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API HelloWorldProcessor::setState(IBStream *state) {
  if (!state)
    return kResultFalse;

  // called when we load a preset or project, the model has to be reloaded

  IBStreamer streamer(state, kLittleEndian);

  float savedParam1 = 0.f;
  if (streamer.readFloat(savedParam1) == false)
    return kResultFalse;

  int32 savedParam2 = 0;
  if (streamer.readInt32(savedParam2) == false)
    return kResultFalse;

  int32 savedBypass = 0;
  if (streamer.readInt32(savedBypass) == false)
    return kResultFalse;

  mParam1 = savedParam1;
  mParam2 = savedParam2 > 0 ? 1 : 0;
  mBypass = savedBypass > 0;

  return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API HelloWorldProcessor::getState(IBStream *state) {
  // here we need to save the model (preset or project)

  float toSaveParam1 = mParam1;
  int32 toSaveParam2 = mParam2;
  int32 toSaveBypass = mBypass ? 1 : 0;

  IBStreamer streamer(state, kLittleEndian);
  streamer.writeFloat(toSaveParam1);
  streamer.writeInt32(toSaveParam2);
  streamer.writeInt32(toSaveBypass);

  return kResultOk;
}

//------------------------------------------------------------------------
} // namespace Steinberg
