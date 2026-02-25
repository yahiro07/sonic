#pragma once

#include "public.sdk/source/main/pluginfactory.h"
#include "vst3wf/SynthesizerBase.h"

typedef SynthesizerBase *(*SynthInstantiateFn)();

typedef struct _PluginMeta {
  std::string name;
  std::string vendor;
  std::string versin;
  std::string category;
  std::string processorCID;
  std::string controllerCID;
} PluginMeta;

extern SynthInstantiateFn gSynthInstantiateFn;

Steinberg::IPluginFactory *PLUGIN_API GetPluginFactoryInternal(
    SynthInstantiateFn synthInstantiateFn, PluginMeta &meta);

#define VstFactoryResult                                                       \
  SMTG_EXPORT_SYMBOL Steinberg::IPluginFactory *PLUGIN_API