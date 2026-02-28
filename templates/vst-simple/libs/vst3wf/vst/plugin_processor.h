#pragma once

#include "logic/parameter_item.h"
#include "logic/realtime_host_event_queue.h"
#include "modules/event_hub.h"
#include "vst3wf/logic/parameter_definitions_provider.h"
#include "vst3wf/synthesizer_base.h"
#include <public.sdk/source/vst/vstaudioeffect.h>

namespace vst3wf {

class PluginProcessor : public Steinberg::Vst::AudioEffect {
private:
  SynthesizerBase *synthInstance;
  ParameterDefinitionsProvider parameterDefinitionsProvider;
  std::unordered_map<ParamAddress, double> parametersCache;
  RealtimeHostEventQueue realtimeHostEventQueue;
  ProcessorSideMessagingBridge processorSideMessagingBridge;

public:
  PluginProcessor();
  ~PluginProcessor() SMTG_OVERRIDE;

  static Steinberg::FUnknown *createInstance(void * /*context*/) {
    return (Steinberg::Vst::IAudioProcessor *)new PluginProcessor;
  }

  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown *context)
      SMTG_OVERRIDE;
  Steinberg::tresult PLUGIN_API terminate() SMTG_OVERRIDE;

  Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) SMTG_OVERRIDE;

  Steinberg::tresult PLUGIN_API
  setupProcessing(Steinberg::Vst::ProcessSetup &newSetup) SMTG_OVERRIDE;
  Steinberg::tresult PLUGIN_API
  canProcessSampleSize(Steinberg::int32 symbolicSampleSize) SMTG_OVERRIDE;

  Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData &data)
      SMTG_OVERRIDE;

  Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream *state)
      SMTG_OVERRIDE;
  Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream *state)
      SMTG_OVERRIDE;

  Steinberg::tresult PLUGIN_API notify(Steinberg::Vst::IMessage *message)
      SMTG_OVERRIDE;
};

} // namespace vst3wf
