#pragma once

#include "../../../common/logger.h"
#include "../../../core/parameter-registry.h"
#include "../../../core/parameter-store.h"
#include "../support/event-message-bus.h"
#include "../vst_entry/vst_entry_wrapper.h"
#include <public.sdk/source/vst/vstaudioeffect.h>

namespace vst_basis {

using namespace sonic;
using ParamAddress = ParamId;

using namespace sonic_vst;
using namespace Steinberg;
using namespace vst_support;

class PluginProcessor : public Vst::AudioEffect {
private:
  std::unique_ptr<SynthesizerBase> synthInstance{
      gPluginFactoryGlobalHolder.synthInstantiateFn()};
  ParameterRegistry parameterRegistry;
  ParameterStore parameterStore;
  ProcessorSideMessagePort processorSideMessagePort{*this};
  SPSCQueue<UpstreamEvent, 256> upstreamEventQueue;
  SPSCQueue<DownstreamEvent, 256> downstreamEventQueue;

public:
  PluginProcessor() {
    logger.start();
    logger.log("PluginProcessor constructor 1541");
    setControllerClass(gPluginFactoryGlobalHolder.controllerCID);
  }

  ~PluginProcessor() SMTG_OVERRIDE { logger.stop(); }

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
