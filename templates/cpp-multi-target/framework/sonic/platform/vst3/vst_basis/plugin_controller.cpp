#include "./plugin_controller.h"
#include "../support/controller-parameter-helper.h"
#include "../support/processor_state_helper.h"
#include "./plugin-editor-view.h"
#include "sonic/api/synthesizer-base.h"
#include <base/source/fstring.h>
#include <sonic/common/logger.h>
#include <sonic/core/parameter-spec-helper.h>

namespace vst_basis {

using namespace sonic_vst;
using namespace sonic;

using ParameterItem = sonic::ParameterSpecItem;

tresult PLUGIN_API PluginController::initialize(FUnknown *context) {
  tresult result = EditControllerEx1::initialize(context);
  if (result != kResultOk) {
    return result;
  }
  if (result == kResultTrue) {
    auto parameterBuilder = ParameterBuilderImpl();
    synthInstance->setupParameters(parameterBuilder);
    auto parameterItems = parameterBuilder.getItems();
    parameterRegistry.addParameters(parameterItems, 0x7FFFFFFE);
    for (auto &item : parameterItems) {
      addVstControllerParameter(this->parameters, item);
    }
    controllerParameterPortal.startObserve();
  }

  return result;
}

tresult PLUGIN_API PluginController::terminate() {
  controllerParameterPortal.stopObserve();
  return EditControllerEx1::terminate();
}

tresult PLUGIN_API PluginController::setComponentState(IBStream *stream) {
  if (!stream)
    return kResultFalse;

  PersistStateData data;
  auto ok = processorStateHelper_readState(stream, data);
  if (!ok) {
    return kResultFalse;
  }
  for (auto &kv : data.parameters) {
    auto paramItem = parameterRegistry.getParameterItemByParamKey(kv.first);
    if (!paramItem)
      continue;
    auto value = kv.second;
    auto normalizedValue = ParameterSpecHelper::getNormalized(paramItem, value);
    setParamNormalized(paramItem->id, normalizedValue);
  }
  return kResultOk;
}

tresult PLUGIN_API PluginController::setState(IBStream *state) {
  // Here you get the state of the controller

  return kResultTrue;
}

tresult PLUGIN_API PluginController::getState(IBStream *state) {
  // Here you are asked to deliver the state of the controller (if needed)
  // Note: the real state of your plug-in is saved in the processor

  return kResultTrue;
}

IPlugView *PLUGIN_API PluginController::createView(FIDString name) {
  if (FIDStringsEqual(name, Vst::ViewType::kEditor)) {
    auto editorPageUrl = synthInstance->getEditorPageUrl();
    // return createWebViewEditorView(this, &parametersManager, &eventHub,
    //  editorPageUrl);
    auto &controllerFacade = domainController.getControllerFacade();
    return new PluginEditorView(this, controllerFacade, editorPageUrl);
  }
  return nullptr;
}

tresult PLUGIN_API PluginController::setParamNormalized(Vst::ParamID tag,
                                                        Vst::ParamValue value) {
  // logger.log("Project1Controller::setParamNormalized: %d, %f", tag, value);
  // called by host to update your parameters
  tresult result = EditControllerEx1::setParamNormalized(tag, value);
  return result;
}

tresult PLUGIN_API PluginController::getParamStringByValue(
    Vst::ParamID tag, Vst::ParamValue valueNormalized, Vst::String128 string) {
  // called by host to get a string for given normalized value of a specific
  // parameter (without having to set the value!)
  return EditControllerEx1::getParamStringByValue(tag, valueNormalized, string);
}

tresult PLUGIN_API PluginController::getParamValueByString(
    Vst::ParamID tag, Vst::TChar *string, Vst::ParamValue &valueNormalized) {
  // called by host to get a normalized value from a string representation of a
  // specific parameter (without having to set the value!)
  return EditControllerEx1::getParamValueByString(tag, string, valueNormalized);
}

tresult PLUGIN_API PluginController::notify(Vst::IMessage *message) {
  auto consumed = controllerSideMessagePort.applyMessageReceived(message);
  if (consumed) {
    return kResultOk;
  } else {
    return EditControllerEx1::notify(message);
  }
}

} // namespace vst_basis
