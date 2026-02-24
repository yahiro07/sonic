//------------------------------------------------------------------------
// Copyright(c) 2022 Steinberg Media Technologies GmbH.
//------------------------------------------------------------------------

#include "./project1_controller.h"
#include "./logic/parameter_builder_impl.h"
#include "./project1_cids.h"
#include "./wrapper/webview_editor_view.h"
#include <base/source/fstreamer.h>
#include <pluginterfaces/base/ibstream.h>
#include <stdio.h>

namespace Project1 {
using namespace Steinberg;

//------------------------------------------------------------------------
// HelloWorldController Implementation
//------------------------------------------------------------------------
tresult PLUGIN_API Project1Controller::initialize(FUnknown *context) {
  // Here the Plug-in will be instantiated
  logger.log("Project1Controller::initialize");

  //---do not forget to call parent ------
  tresult result = EditControllerEx1::initialize(context);
  if (result != kResultOk) {
    return result;
  }

  // Here you could register some parameters
  if (result == kResultTrue) {
    //---Create Parameters------------
    auto parameterBuilder = ParameterBuilderImpl();
    synthInstance->setupParameters(parameterBuilder);
    auto parameterItems = parameterBuilder.getItems();
    parametersManager.addParameters(parameterItems);
  }

  return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Project1Controller::terminate() {
  // Here the Plug-in will be de-instantiated, last possibility to remove some
  // memory!

  //---do not forget to call parent ------
  return EditControllerEx1::terminate();
}

//------------------------------------------------------------------------
tresult PLUGIN_API Project1Controller::setComponentState(IBStream *state) {
  // Here you get the state of the component (Processor part)
  if (!state)
    return kResultFalse;

  IBStreamer streamer(state, kLittleEndian);

  float savedParam1 = 0.f;
  if (streamer.readFloat(savedParam1) == false)
    return kResultFalse;
  setParamNormalized(Project1Params::kParamVolId, savedParam1);

  int8 savedParam2 = 0;
  if (streamer.readInt8(savedParam2) == false)
    return kResultFalse;
  setParamNormalized(Project1Params::kParamOnId, savedParam2);

  // read the bypass
  int32 bypassState;
  if (streamer.readInt32(bypassState) == false)
    return kResultFalse;
  setParamNormalized(kBypassId, bypassState ? 1 : 0);

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
    return createWebViewEditorView(this, &parametersManager);
  }
  return nullptr;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Project1Controller::setParamNormalized(
    Vst::ParamID tag, Vst::ParamValue value) {
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

//------------------------------------------------------------------------
} // namespace Project1
