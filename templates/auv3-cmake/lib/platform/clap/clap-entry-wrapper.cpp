#include "./clap-entry-wrapper.h"
#include "./clap-rootage.h"
#include "./entry-controller.h"

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
    auto &desc = clapRootage_getPluginDescriptor();
    overwriteDescriptor(desc, meta);
    clapRootage_setEntryControllerInstantiateFn(
        [synthInitializer]() -> EntryController * {
          auto synth = synthInitializer();
          if (!synth) {
            return nullptr;
          }
          return EntryController::create(synth);
        });
    return clapRootage_getClapPluginEntry();
  }();
  return clapPlugin;
}