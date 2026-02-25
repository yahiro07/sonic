#include "./vst_entry_wrapper.h"
#include "./project1_cids.h"
#include "./project1_controller.h"
#include "./project1_processor.h"
#include "./version.h"
#include "public.sdk/source/main/pluginfactory.h"

#define stringPluginName "Project1"

using namespace Steinberg::Vst;
using namespace Steinberg;
using namespace Project1;

SynthInstantiateFn gSynthInstantiateFn = nullptr;

IPluginFactory *PLUGIN_API GetPluginFactoryInternal(
    SynthInstantiateFn synthInstantiateFn, PluginMeta &meta) {

  gSynthInstantiateFn = synthInstantiateFn;

  // TODO: affect meta to definitions below
  if (!gPluginFactory) {
    static PFactoryInfo factoryInfo("MyCompany", "www.mycompany.com",
                                    "mailto:info@mycompany.com",
                                    Vst::kDefaultFactoryFlags);
    gPluginFactory = new CPluginFactory(factoryInfo);

    //---First Plug-in included in this factory-------
    // its kVstAudioEffectClass component
    {
      const TUID lcid = INLINE_UID(
          kProject1ProcessorUID.getLong1(), kProject1ProcessorUID.getLong2(),
          kProject1ProcessorUID.getLong3(), kProject1ProcessorUID.getLong4());
      static PClassInfo2 componentClass(
          lcid,
          PClassInfo::kManyInstances, // cardinality
          kVstAudioEffectClass,       // the component category
          stringPluginName,           // here the Plug-in name
          Vst::kDistributable,        // class flags
          Project1VST3Category,       // Subcategory for this Plug-in
          nullptr,                    // vendor (use factory default)
          FULL_VERSION_STR,           // Plug-in version
          kVstVersionString           // the VST 3 SDK version
      );
      gPluginFactory->registerClass(&componentClass,
                                    Project1Processor::createInstance);
    }

    // its kVstComponentControllerClass component
    {
      const TUID lcid = INLINE_UID(
          kProject1ControllerUID.getLong1(), kProject1ControllerUID.getLong2(),
          kProject1ControllerUID.getLong3(), kProject1ControllerUID.getLong4());
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
