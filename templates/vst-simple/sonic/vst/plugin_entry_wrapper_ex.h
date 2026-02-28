
#include "../vst_entry/vst_entry_wrapper.h"
#include "./plugin_controller.h"
#include "./plugin_processor.h"

namespace vst3wf_plugin {

inline Steinberg::IPluginFactory *PLUGIN_API GetPluginFactoryInternal(
    SynthInstantiateFn synthInstantiateFn, PluginMeta &meta) {
  return GetPluginFactoryInternal(synthInstantiateFn,
                                  PluginProcessor::createInstance,
                                  PluginController::createInstance, meta);
}

} // namespace vst3wf_plugin