//------------------------------------------------------------------------
// Copyright(c) 2022 Steinberg Media Technologies GmbH.
//------------------------------------------------------------------------

#pragma once

#include "logic/parameter_item.h"
#include "logic/realtime_host_event_queue.h"
#include "modules/event_hub.h"
#include "vst3wf/SynthesizerBase.h"
#include "vst3wf/logic/parameter_definitions_provider.h"
#include <public.sdk/source/vst/vstaudioeffect.h>

namespace Project1 {

//------------------------------------------------------------------------
//  Project1Processor
//------------------------------------------------------------------------
class Project1Processor : public Steinberg::Vst::AudioEffect {
private:
  SynthesizerBase *synthInstance;
  Amx::ParameterDefinitionsProvider parameterDefinitionsProvider;
  std::unordered_map<Amx::ParamAddress, double> parametersCache;
  Amx::RealtimeHostEventQueue realtimeHostEventQueue;
  vst3wf::ProcessorSideMessagingBridge processorSideMessagingBridge;

public:
  Project1Processor();
  ~Project1Processor() SMTG_OVERRIDE;

  // Create function
  static Steinberg::FUnknown *createInstance(void * /*context*/) {
    return (Steinberg::Vst::IAudioProcessor *)new Project1Processor;
  }

  //--- ---------------------------------------------------------------------
  // AudioEffect overrides:
  //--- ---------------------------------------------------------------------
  /** Called at first after constructor */
  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown *context)
      SMTG_OVERRIDE;

  /** Called at the end before destructor */
  Steinberg::tresult PLUGIN_API terminate() SMTG_OVERRIDE;

  /** Switch the Plug-in on/off */
  Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) SMTG_OVERRIDE;

  /** Will be called before any process call */
  Steinberg::tresult PLUGIN_API
  setupProcessing(Steinberg::Vst::ProcessSetup &newSetup) SMTG_OVERRIDE;

  /** Asks if a given sample size is supported see SymbolicSampleSizes. */
  Steinberg::tresult PLUGIN_API
  canProcessSampleSize(Steinberg::int32 symbolicSampleSize) SMTG_OVERRIDE;

  /** Here we go...the process call */
  Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData &data)
      SMTG_OVERRIDE;

  /** For persistence */
  Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream *state)
      SMTG_OVERRIDE;
  Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream *state)
      SMTG_OVERRIDE;

  Steinberg::tresult PLUGIN_API notify(Steinberg::Vst::IMessage *message)
      SMTG_OVERRIDE;
};

//------------------------------------------------------------------------
} // namespace Project1
