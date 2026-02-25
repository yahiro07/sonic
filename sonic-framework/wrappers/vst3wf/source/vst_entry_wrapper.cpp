#include "./vst_entry_wrapper.h"
#include "./project1_controller.h"
#include "./project1_processor.h"
#include "projectversion.h"
#include "public.sdk/source/main/pluginfactory.h"
#include "tuid_helper.h"

using namespace Steinberg::Vst;
using namespace Steinberg;
using namespace Project1;

PluginFactoryGlobalHolder gPluginFactoryGlobalHolder = {
    .gSynthInstantiateFn = nullptr,
    .processorCID = {},
    .controllerCID = {},
};

IPluginFactory *PLUGIN_API GetPluginFactoryInternal(
    SynthInstantiateFn synthInstantiateFn, PluginMeta &meta) {

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
          PClassInfo::kManyInstances, // cardinality
          kVstAudioEffectClass,       // the component category
          meta.name.c_str(),          // here the Plug-in name
          Vst::kDistributable,        // class flags
          meta.category.c_str(),      // Subcategory for this Plug-in
          nullptr,                    // vendor (use factory default)
          FULL_VERSION_STR,           // Plug-in version
          kVstVersionString           // the VST 3 SDK version
      );
      gPluginFactory->registerClass(&componentClass,
                                    Project1Processor::createInstance);
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
          FULL_VERSION_STR,                   // Plug-in version
          kVstVersionString                   // the VST 3 SDK version
      );
      gPluginFactory->registerClass(&componentClass,
                                    Project1Controller::createInstance);
    }
  } else {
    gPluginFactory->addRef();
  }
  return gPluginFactory;
}
