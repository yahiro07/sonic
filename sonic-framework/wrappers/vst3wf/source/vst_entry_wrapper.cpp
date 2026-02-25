#include "./vst_entry_wrapper.h"
#include "./project1_controller.h"
#include "./project1_processor.h"
#include "projectversion.h"
#include "public.sdk/source/main/pluginfactory.h"
#include "tuid_helper.h"

#define stringPluginName "Project1"

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

  // TODO: affect meta to definitions below
  if (!gPluginFactory) {
    static PFactoryInfo factoryInfo("MyCompany", "www.mycompany.com",
                                    "mailto:info@mycompany.com",
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
          stringPluginName,           // here the Plug-in name
          Vst::kDistributable,        // class flags
          "Fx",                       // Subcategory for this Plug-in
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
          PClassInfo::kManyInstances,    // cardinality
          kVstComponentControllerClass,  // the Controller category
          stringPluginName "Controller", // controller name
          0,                             // class flags
          "",                            // Subcategory
          nullptr,                       // vendor (use factory default)
          FULL_VERSION_STR,              // Plug-in version
          kVstVersionString              // the VST 3 SDK version
      );
      gPluginFactory->registerClass(&componentClass,
                                    Project1Controller::createInstance);
    }
  } else {
    gPluginFactory->addRef();
  }
  return gPluginFactory;
}
