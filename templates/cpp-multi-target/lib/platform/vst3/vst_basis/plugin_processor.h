#pragma once

#include "../logic/parameter_definitions_provider.h"
#include "../logic/parameter_item.h"
#include "../logic/realtime_host_event_queue.h"
#include "../modules/event_hub.h"
#include <public.sdk/source/vst/vstaudioeffect.h>

namespace vst_basis {

using namespace sonic_vst;
using namespace Steinberg;

class PluginProcessor : public Vst::AudioEffect {
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

  tresult PLUGIN_API initialize(FUnknown *context) SMTG_OVERRIDE;
  tresult PLUGIN_API terminate() SMTG_OVERRIDE;

  tresult PLUGIN_API setActive(Steinberg::TBool state) SMTG_OVERRIDE;

  tresult PLUGIN_API setupProcessing(Vst::ProcessSetup &newSetup) SMTG_OVERRIDE;
  tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize)
      SMTG_OVERRIDE;

  tresult PLUGIN_API process(Vst::ProcessData &data) SMTG_OVERRIDE;

  tresult PLUGIN_API getState(IBStream *state) SMTG_OVERRIDE;
  tresult PLUGIN_API setState(IBStream *state) SMTG_OVERRIDE;

  tresult PLUGIN_API notify(Vst::IMessage *message) SMTG_OVERRIDE;
};

} // namespace vst_basis
