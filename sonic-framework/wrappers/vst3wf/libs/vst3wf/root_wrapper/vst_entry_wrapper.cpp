#include "./vst_entry_wrapper.h"
#include "./tuid_helper.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "public.sdk/source/main/pluginfactory.h"

using namespace Steinberg::Vst;
using namespace Steinberg;

namespace vst3wf {

PluginFactoryGlobalHolder gPluginFactoryGlobalHolder = {
    .gSynthInstantiateFn = nullptr,
    .processorCID = {},
    .controllerCID = {},
};

IPluginFactory *PLUGIN_API GetPluginFactoryInternal(
    SynthInstantiateFn synthInstantiateFn,
    FUnknown *(*processorCreateInstanceFn)(void *),
    FUnknown *(*controllerCreateInstanceFn)(void *), PluginMeta &meta) {

  gPluginFactoryGlobalHolder.gSynthInstantiateFn = synthInstantiateFn;
  loadTUIDFromGUIDString(gPluginFactoryGlobalHolder.processorCID,
                         meta.processorCID);
  loadTUIDFromGUIDString(gPluginFactoryGlobalHolder.controllerCID,
                         meta.controllerCID);

  if (!gPluginFactory) {
    static PFactoryInfo factoryInfo(meta.vendor.c_str(), meta.url.c_str(),
                                    meta.email.c_str(),
                                    Vst::kDefaultFactoryFlags);
    gPluginFactory = new CPluginFactory(factoryInfo);

    //---First Plug-in included in this factory-------
    // its kVstAudioEffectClass component
    {
      TUID lcid;
      loadTUIDFromGUIDString(lcid, meta.processorCID);
      static PClassInfo2 componentClass(
          lcid,
          PClassInfo::kManyInstances,  // cardinality
          kVstAudioEffectClass,        // the component category
          meta.name.c_str(),           // here the Plug-in name
          Vst::kDistributable,         // class flags
          meta.category.c_str(),       // Subcategory for this Plug-in
          nullptr,                     // vendor (use factory default)
          meta.fullVersionStr.c_str(), // Plug-in version
          kVstVersionString            // the VST 3 SDK version
      );
      gPluginFactory->registerClass(&componentClass, processorCreateInstanceFn);
    }

    // its kVstComponentControllerClass component
    {
      TUID lcid;
      loadTUIDFromGUIDString(lcid, meta.controllerCID);
      static PClassInfo2 componentClass(
          lcid,
          PClassInfo::kManyInstances,         // cardinality
          kVstComponentControllerClass,       // the Controller category
          (meta.name + "Controller").c_str(), // controller name
          0,                                  // class flags
          "",                                 // Subcategory
          nullptr,                            // vendor (use factory default)
          meta.fullVersionStr.c_str(),        // Plug-in version
          kVstVersionString                   // the VST 3 SDK version
      );
      gPluginFactory->registerClass(&componentClass,
                                    controllerCreateInstanceFn);
    }
  } else {
    gPluginFactory->addRef();
  }
  return gPluginFactory;
}

} // namespace vst3wf
