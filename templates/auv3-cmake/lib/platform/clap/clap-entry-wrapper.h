#pragma once
#include "../../domain/interfaces.h"
#include <clap/clap.h>

struct PluginMeta {
  const char *id;
  const char *name;
  const char *vendor;
  const char *url;
  const char *manualUrl;
  const char *supportUrl;
  const char *version;
  const char *description;
};

typedef sonic::IPluginSynthesizer *(*SynthesizerInitializerFn)();

const clap_plugin_entry_t &
createClapPluginEntry(SynthesizerInitializerFn synthInitializer,
                      const PluginMeta &meta);
