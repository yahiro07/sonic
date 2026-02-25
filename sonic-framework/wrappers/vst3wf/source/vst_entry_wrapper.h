#pragma once

#include "public.sdk/source/main/pluginfactory.h"
#include "vst3wf/SynthesizerBase.h"

typedef SynthesizerBase *(*SynthInstantiateFn)();

typedef struct _PluginMeta {
  std::string name;
  std::string category;
  std::string vendor;
  std::string url;
  std::string email;
  std::string processorCID;
  std::string controllerCID;
} PluginMeta;

typedef struct _PluginFactoryGlobalHolder {
  SynthInstantiateFn gSynthInstantiateFn;
  Steinberg::TUID processorCID;
  Steinberg::TUID controllerCID;
} PluginFactoryGlobalHolder;

extern PluginFactoryGlobalHolder gPluginFactoryGlobalHolder;

Steinberg::IPluginFactory *PLUGIN_API GetPluginFactoryInternal(
    SynthInstantiateFn synthInstantiateFn, PluginMeta &meta);

#define VstFactoryResult                                                       \
  SMTG_EXPORT_SYMBOL Steinberg::IPluginFactory *PLUGIN_API