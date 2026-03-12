#include "./plugin_controller.h"
#include "../../../core/parameter-builder-impl.h"
#include "../../../core/parameter-spec-helper.h"
#include "../modules/processor_state_helper.h"
#include "../modules/webview_editor_view.h"

namespace vst_basis {

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
    parametersManager.addParameters(parameterItems);
    parametersManager.startObserve();
  }

  return result;
}

tresult PLUGIN_API PluginController::terminate() {
  parametersManager.stopObserve();
  return EditControllerEx1::terminate();
}

tresult PLUGIN_API PluginController::setComponentState(IBStream *state) {
  logger.log("PluginController::setComponentState");
  if (!state)
    return kResultFalse;

  ProcessorState processorState;
  auto ok = processorStateHelper_readState(state, processorState);
  if (!ok) {
    return kResultFalse;
  }
  for (auto &kv : processorState.parameters) {
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
    return createWebViewEditorView(this, &parametersManager, &eventHub,
                                   editorPageUrl);
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
  logger.log("PluginController::notify");
  auto consumed = eventHub.notifyFromEditController(message);
  if (consumed) {
    return kResultOk;
  } else {
    return EditControllerEx1::notify(message);
  }
}

} // namespace vst_basis
