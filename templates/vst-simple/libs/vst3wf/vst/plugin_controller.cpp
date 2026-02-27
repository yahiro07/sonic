//------------------------------------------------------------------------
// Copyright(c) 2022 Steinberg Media Technologies GmbH.
//------------------------------------------------------------------------

#include "./plugin_controller.h"
#include "../modules/processor_state_helper.h"
#include "pluginterfaces/base/funknown.h"
#include "vst3wf/logic/parameter_builder_impl.h"
#include "vst3wf/logic/parameter_item_helper.h"
#include "vst3wf/modules/webview_editor_view.h"
#include <base/source/fstreamer.h>
#include <glaze/glaze.hpp>
#include <pluginterfaces/base/ibstream.h>
#include <stdio.h>

namespace vst3wf {
using namespace Steinberg;

//------------------------------------------------------------------------
// PluginController Implementation
//------------------------------------------------------------------------
tresult PLUGIN_API PluginController::initialize(FUnknown *context) {
  // Here the Plug-in will be instantiated
  vst3wf::logger.log("PluginController::initialize");

  //---do not forget to call parent ------
  tresult result = EditControllerEx1::initialize(context);
  if (result != kResultOk) {
    return result;
  }

  // Here you could register some parameters
  if (result == kResultTrue) {
    //---Create Parameters------------
    auto parameterBuilder = Amx::ParameterBuilderImpl();
    synthInstance->setupParameters(parameterBuilder);
    auto parameterItems = parameterBuilder.getItems();
    parameterDefinitionsProvider.addParameters(parameterItems);
    parametersManager.addParameters(parameterItems);
    parametersManager.startObserve();
  }

  return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API PluginController::terminate() {
  // Here the Plug-in will be de-instantiated, last possibility to remove some
  // memory!
  parametersManager.stopObserve();

  //---do not forget to call parent ------
  return EditControllerEx1::terminate();
}

//------------------------------------------------------------------------
tresult PLUGIN_API PluginController::setComponentState(IBStream *state) {
  // Here you get the state of the component (Processor part)
  vst3wf::logger.log("PluginController::setComponentState");
  if (!state)
    return kResultFalse;

  vst3wf::ProcessorState processorState;
  auto ok = processorStateHelper_readState(state, processorState);
  if (!ok) {
    return kResultFalse;
  }
  for (auto &kv : processorState.parameters) {
    auto paramItem =
        parameterDefinitionsProvider.getParameterItemByIdentifier(kv.first);
    if (!paramItem)
      continue;
    auto value = kv.second;
    auto normalizedValue =
        Amx::ParameterItemHelper::getNormalized(paramItem, value);
    setParamNormalized(paramItem->address, normalizedValue);
  }
  return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API PluginController::setState(IBStream *state) {
  // Here you get the state of the controller

  return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API PluginController::getState(IBStream *state) {
  // Here you are asked to deliver the state of the controller (if needed)
  // Note: the real state of your plug-in is saved in the processor

  return kResultTrue;
}

//------------------------------------------------------------------------
IPlugView *PLUGIN_API PluginController::createView(FIDString name) {
  // Here the Host wants to open your editor (if you have one)
  if (FIDStringsEqual(name, Vst::ViewType::kEditor)) {
    // create your editor here and return a IPlugView ptr of it
    // return new VSTGUI::VST3Editor(this, "view", "helloworldeditor.uidesc");
    return createWebViewEditorView(this, &parametersManager, &eventHub);
  }
  return nullptr;
}

//------------------------------------------------------------------------
tresult PLUGIN_API PluginController::setParamNormalized(Vst::ParamID tag,
                                                        Vst::ParamValue value) {
  // logger.log("Project1Controller::setParamNormalized: %d, %f", tag, value);
  // called by host to update your parameters
  tresult result = EditControllerEx1::setParamNormalized(tag, value);
  return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API PluginController::getParamStringByValue(
    Vst::ParamID tag, Vst::ParamValue valueNormalized, Vst::String128 string) {
  // called by host to get a string for given normalized value of a specific
  // parameter (without having to set the value!)
  return EditControllerEx1::getParamStringByValue(tag, valueNormalized, string);
}

//------------------------------------------------------------------------
tresult PLUGIN_API PluginController::getParamValueByString(
    Vst::ParamID tag, Vst::TChar *string, Vst::ParamValue &valueNormalized) {
  // called by host to get a normalized value from a string representation of a
  // specific parameter (without having to set the value!)
  return EditControllerEx1::getParamValueByString(tag, string, valueNormalized);
}

tresult PLUGIN_API PluginController::notify(Vst::IMessage *message) {
  vst3wf::logger.log("PluginController::notify");
  auto consumed = eventHub.notifyFromEditController(message);
  if (consumed) {
    return kResultOk;
  } else {
    return EditControllerEx1::notify(message);
  }
}

//------------------------------------------------------------------------
} // namespace vst3wf
