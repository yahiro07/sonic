//------------------------------------------------------------------------
// Copyright(c) 2022 Steinberg Media Technologies GmbH.
//------------------------------------------------------------------------

#include "./project1_controller.h"
#include "./state_format.h"
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

  constexpr int32 kStateMagic = 0x534F4E43; // 'S''O''N''C'
  constexpr int32 kMaxStateBytes = 1024 * 1024;

  IBStreamer streamer(state, kLittleEndian);
  int32 firstWord = 0;
  if (!streamer.readInt32(firstWord))
    return kResultFalse;

  int32 stateVersion = 0;
  int32 size = 0;
  if (firstWord == kStateMagic) {
    if (!streamer.readInt32(stateVersion))
      return kResultFalse;
    if (!streamer.readInt32(size))
      return kResultFalse;
  } else {
    return kResultFalse;
  }

  if (size < 0 || size > kMaxStateBytes) {
    vst3wf::logger.log("invalid state size: %d", size);
    return kResultFalse;
  }

  std::string jsonStr;
  jsonStr.resize(static_cast<size_t>(size));
  if (size > 0) {
    const auto bytesRead = streamer.readRaw(jsonStr.data(), size);
    if (bytesRead != size) {
      vst3wf::logger.log("failed reading state bytes: %d/%d",
                         static_cast<int32>(bytesRead), size);
      return kResultFalse;
    }
  } else {
    // Accept empty state (keep defaults)
    vst3wf::logger.log("empty state (size=0)");
    return kResultOk;
  }
  ProcessorState processorState{};
  processorState.parametersVersion = 0;

  // Preferred format: {"parametersVersion": N, "parameters": {"id": v, ...}}
  auto ec = glz::read_jsonc(processorState, jsonStr);
  if (ec) {
    // Backward-compat: legacy format is just {"id": v, ...}
    std::unordered_map<std::string, double> legacyParameters;
    auto legacyEc = glz::read_jsonc(legacyParameters, jsonStr);
    if (legacyEc) {
      vst3wf::logger.log("error reading json: %s",
                         glz::format_error(ec, jsonStr).c_str());
      return kResultFalse;
    }
    processorState.parameters = std::move(legacyParameters);
  }
  vst3wf::logger.log("jsonStr: %s", jsonStr.c_str());

  if (stateVersion != 0) {
    vst3wf::logger.log("state version: %d", stateVersion);
  }

  vst3wf::logger.log("parametersVersion: %d", processorState.parametersVersion);

  for (auto &kv : processorState.parameters) {
    auto paramItem =
        parameterDefinitionsProvider.getParameterItemByIdentifier(kv.first);
    if (!paramItem) {
      vst3wf::logger.log("unknown parameter identifier in state: %s",
                         kv.first.c_str());
      continue;
    }
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
    return createWebViewEditorView(this, &parametersManager);
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

//------------------------------------------------------------------------
} // namespace Project1
