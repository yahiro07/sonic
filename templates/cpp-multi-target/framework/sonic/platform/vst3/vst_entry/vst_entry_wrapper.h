#pragma once

#include <public.sdk/source/main/pluginfactory.h>
#include <sonic/api/synthesizer-base.h>

namespace sonic_vst {

typedef sonic::SynthesizerBase *(*SynthInstantiateFn)();

typedef struct _PluginMeta {
  std::string name;
  std::string category;
  std::string vendor;
  std::string url;
  std::string email;
  std::string processorCID;
  std::string controllerCID;
  std::string fullVersionStr;
} PluginMeta;

typedef struct _PluginFactoryGlobalHolder {
  SynthInstantiateFn synthInstantiateFn;
  Steinberg::TUID processorCID;
  Steinberg::TUID controllerCID;
} PluginFactoryGlobalHolder;

extern PluginFactoryGlobalHolder gPluginFactoryGlobalHolder;

Steinberg::IPluginFactory *PLUGIN_API GetPluginFactoryInternal(
    SynthInstantiateFn synthInstantiateFn,
    Steinberg::FUnknown *(*processorCreateInstanceFn)(void *),
    Steinberg::FUnknown *(*controllerCreateInstanceFn)(void *),
    PluginMeta &meta);

#define VstFactoryResult                                                       \
  SMTG_EXPORT_SYMBOL Steinberg::IPluginFactory *PLUGIN_API

} // namespace sonic_vst