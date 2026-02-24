#pragma once

#include <functional>
#include <public.sdk/source/vst/vsteditcontroller.h>

using namespace Steinberg;

class EditControllerParameterChangeNotifier : public IDependent {
public:
  void start(Vst::EditController *controller,
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

  void stop() {
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

  void PLUGIN_API update(FUnknown *changedUnknown, int32 message) override {
    if (receiver) {
      FUnknownPtr<Vst::Parameter> param(changedUnknown);
      if (param) {
        receiver(param->getInfo().id, param->getNormalized());
      }
    }
  }

  tresult PLUGIN_API queryInterface(const TUID _iid, void **obj) override {
    QUERY_INTERFACE(_iid, obj, IDependent::iid, IDependent)
    QUERY_INTERFACE(_iid, obj, FUnknown::iid, IDependent)
    return kNoInterface;
  }

  uint32 PLUGIN_API addRef() override { return 1; }
  uint32 PLUGIN_API release() override { return 1; }

private:
  Vst::EditController *controller = nullptr;
  std::function<void(Vst::ParamID paramId, double value)> receiver;
};