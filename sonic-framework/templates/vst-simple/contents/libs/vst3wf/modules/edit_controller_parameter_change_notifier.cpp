#include "./edit_controller_parameter_change_notifier.h"
#include <functional>
#include <public.sdk/source/vst/vsteditcontroller.h>

namespace vst3wf {

using namespace Steinberg;

void EditControllerParameterChangeNotifier::start(
    Vst::EditController *controller,
    std::function<void(Vst::ParamID paramId, double value)> receiver) {
  this->controller = controller;
  this->receiver = receiver;
  if (controller) {
    int32 count = controller->getParameterCount();
    for (int32 i = 0; i < count; i++) {
      Vst::ParameterInfo info;
      if (controller->getParameterInfo(i, info) == kResultOk) {
        if (auto *param = controller->getParameterObject(info.id)) {
          param->addDependent(this);
        }
      }
    }
  }
}

void EditControllerParameterChangeNotifier::stop() {
  if (controller) {
    int32 count = controller->getParameterCount();
    for (int32 i = 0; i < count; i++) {
      Vst::ParameterInfo info;
      if (controller->getParameterInfo(i, info) == kResultOk) {
        if (auto *param = controller->getParameterObject(info.id)) {
          param->removeDependent(this);
        }
      }
    }
  }
  controller = nullptr;
  receiver = nullptr;
}

void PLUGIN_API EditControllerParameterChangeNotifier::update(
    FUnknown *changedUnknown, int32 message) {
  if (receiver) {
    FUnknownPtr<Vst::Parameter> param(changedUnknown);
    if (param) {
      receiver(param->getInfo().id, param->getNormalized());
    }
  }
}

} // namespace vst3wf