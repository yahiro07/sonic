
#include "./plugin_controller.h"
#include "./plugin_processor.h"
#include "vst3wf/vst_entry/vst_entry_wrapper.h"

namespace vst3wf_plugin {

inline Steinberg::IPluginFactory *PLUGIN_API GetPluginFactoryInternal(
    SynthInstantiateFn synthInstantiateFn, PluginMeta &meta) {
  return GetPluginFactoryInternal(synthInstantiateFn,
                                  PluginProcessor::createInstance,
                                  PluginController::createInstance, meta);
}

} // namespace vst3wf_plugin