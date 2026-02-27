//------------------------------------------------------------------------
// Copyright(c) 2022 Steinberg Media Technologies GmbH.
//------------------------------------------------------------------------

#include "./project1_controller.h"
#include "../modules/processor_state_helper.h"
#include "pluginterfaces/base/funknown.h"
#include "vst3wf/logic/parameter_builder_impl.h"
#include "vst3wf/logic/parameter_item_helper.h"
#include "vst3wf/modules/webview_editor_view.h"
#include <base/source/fstreamer.h>
#include <glaze/glaze.hpp>
#include <pluginterfaces/base/ibstream.h>
#include <stdio.h>

namespace Project1 {
using namespace Steinberg;

//------------------------------------------------------------------------
// HelloWorldController Implementation
//------------------------------------------------------------------------
tresult PLUGIN_API Project1Controller::initialize(FUnknown *context) {
  // Here the Plug-in will be instantiated
  vst3wf::logger.log("Project1Controller::initialize");

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
tresult PLUGIN_API Project1Controller::terminate() {
  // Here the Plug-in will be de-instantiated, last possibility to remove some
  // memory!
  parametersManager.stopObserve();

  //---do not forget to call parent ------
  return EditControllerEx1::terminate();
}

//------------------------------------------------------------------------
tresult PLUGIN_API Project1Controller::setComponentState(IBStream *state) {
  // Here you get the state of the component (Processor part)
  vst3wf::logger.log("Project1Controller::setComponentState");
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
tresult PLUGIN_API Project1Controller::setState(IBStream *state) {
  // Here you get the state of the controller

  return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Project1Controller::getState(IBStream *state) {
  // Here you are asked to deliver the state of the controller (if needed)
  // Note: the real state of your plug-in is saved in the processor

  return kResultTrue;
}

//------------------------------------------------------------------------
IPlugView *PLUGIN_API Project1Controller::createView(FIDString name) {
  // Here the Host wants to open your editor (if you have one)
  if (FIDStringsEqual(name, Vst::ViewType::kEditor)) {
    // create your editor here and return a IPlugView ptr of it
    // return new VSTGUI::VST3Editor(this, "view", "helloworldeditor.uidesc");
    return createWebViewEditorView(this, &parametersManager, &eventHub);
  }
  return nullptr;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Project1Controller::setParamNormalized(
    Vst::ParamID tag, Vst::ParamValue value) {
  // logger.log("Project1Controller::setParamNormalized: %d, %f", tag, value);
  // called by host to update your parameters
  tresult result = EditControllerEx1::setParamNormalized(tag, value);
  return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Project1Controller::getParamStringByValue(
    Vst::ParamID tag, Vst::ParamValue valueNormalized, Vst::String128 string) {
  // called by host to get a string for given normalized value of a specific
  // parameter (without having to set the value!)
  return EditControllerEx1::getParamStringByValue(tag, valueNormalized, string);
}

//------------------------------------------------------------------------
tresult PLUGIN_API Project1Controller::getParamValueByString(
    Vst::ParamID tag, Vst::TChar *string, Vst::ParamValue &valueNormalized) {
  // called by host to get a normalized value from a string representation of a
  // specific parameter (without having to set the value!)
  return EditControllerEx1::getParamValueByString(tag, string, valueNormalized);
}

tresult PLUGIN_API Project1Controller::notify(Vst::IMessage *message) {
  vst3wf::logger.log("Project1Controller::notify");
  eventHub.notifyFromEditController(message);
  return kResultOk;
}

//------------------------------------------------------------------------
} // namespace Project1
