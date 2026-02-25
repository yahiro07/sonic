#include "../MySynthesizer.h"
#include "./project1_controller.h"
#include "./project1_processor.h"
#include "projectversion.h"
#include "vst3wf/root_wrapper/vst_entry_wrapper.h"

using namespace Project1;

static vst3wf::PluginMeta pluginMeta{
    .name = "MyPlugin",
    .category = "Instrument",
    .vendor = "MyCompany",
    .url = "www.mycompany.com",
    .email = "info@mycompany.com",
    .processorCID = "__TEMPLATE_VST3_PROCESSOR_CID__",
    .controllerCID = "__TEMPLATE_VST3_CONTROLLER_CID__",
    .fullVersionStr = FULL_VERSION_STR,
};

VstFactoryResult GetPluginFactory() {
  return vst3wf::GetPluginFactoryInternal(
      createSynthesizerInstance, Project1Processor::createInstance,
      Project1Controller::createInstance, pluginMeta);
}