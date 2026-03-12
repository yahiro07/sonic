#pragma once

#include "../../../core/parameter-registry.h"
#include "../../../core/parameter-spec-item.h"
#include "../modules/event_hub.h"
#include "../modules/realtime_host_event_queue.h"
#include <public.sdk/source/vst/vstaudioeffect.h>

namespace vst_basis {

using namespace sonic;
using ParamAddress = ParamId;

using namespace sonic_vst;
using namespace Steinberg;

class PluginProcessor : public Vst::AudioEffect {
private:
  SynthesizerBase *synthInstance;
  ParameterRegistry parameterRegistry;
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
