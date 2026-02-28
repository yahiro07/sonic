#include "./project1_synthesizer.h"
#include "projectversion.h"
#include "sonic/vst_basis/plugin_controller.h"
#include "sonic/vst_basis/plugin_processor.h"
#include "sonic/vst_entry/vst_entry_wrapper.h"

static sonic_vst::PluginMeta pluginMeta{
    .name = "MyPlugin",
    .category = "Instrument",
    .vendor = "MyCompany",
    .url = "www.mycompany.com",
    .email = "info@mycompany.com",
    .processorCID = "E458F80E-DEED-40DB-AD59-2C739A7DA7A0",
    .controllerCID = "6BAD2674-0204-4522-8971-58C6296A4552",
    .fullVersionStr = FULL_VERSION_STR,
};

VstFactoryResult GetPluginFactory() {
  return sonic_vst::GetPluginFactoryInternal(
      createSynthesizerInstance, vst_basis::PluginProcessor::createInstance,
      vst_basis::PluginController::createInstance, pluginMeta);
}