//------------------------------------------------------------------------
// Copyright(c) 2022 Steinberg Media Technologies GmbH.
//------------------------------------------------------------------------

#include "project1_controller.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include "project1_cids.h"
#include "stdio.h"

// #include "vstgui/plugin-bindings/vst3editor.h"

#include "webview/webview_editor_view.h"

using namespace Steinberg;

namespace Steinberg {

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
    parameters.addParameter(STR16("Bypass"), nullptr, 1, 0,
                            Vst::ParameterInfo::kCanAutomate |
                                Vst::ParameterInfo::kIsBypass,
                            Project1Params::kBypassId);

    parameters.addParameter(STR16("Parameter 1"), STR16("dB"), 0, .5,
                            Vst::ParameterInfo::kCanAutomate,
                            Project1Params::kParamVolId, 0, STR16("Param1"));

    parameters.addParameter(STR16("Parameter 2"), STR16("On/Off"), 1, 1.,
                            Vst::ParameterInfo::kCanAutomate,
                            Project1Params::kParamOnId, 0, STR16("Param2"));
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
    return new WebViewEditorView(this);
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
} // namespace Steinberg
