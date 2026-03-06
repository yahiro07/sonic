#include "./clap_wrapper.h"
#include "../rootage/clap_rootage.h"
#include "./plug_basis_impl.h"

static void overwriteDescriptor(clap_plugin_descriptor_t &desc,
                                const PluginMeta &meta) {
  desc.id = meta.id;
  desc.name = meta.name;
  desc.vendor = meta.vendor;
  desc.url = meta.url;
  desc.manual_url = meta.manualUrl;
  desc.support_url = meta.supportUrl;
  desc.version = meta.version;
  desc.description = meta.description;
}

const clap_plugin_entry_t &
createClapPluginEntry(SynthesizerInitializerFn synthInitializer,
                      const PluginMeta &meta) {
  static const clap_plugin_entry_t clapPlugin = [synthInitializer, meta]() {
    auto desc = clapRootage_getPluginDescriptor();
    overwriteDescriptor(desc, meta);
    clapRootage_setPluginBasisInstantiateFn([synthInitializer]() {
      auto synth = synthInitializer();
      return new PlugBasisImpl(*synth);
    });
    return clapRootage_getClapPluginEntry();
  }();
  return clapPlugin;
}