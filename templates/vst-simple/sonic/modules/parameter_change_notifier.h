#pragma once

#include <functional>
#include <public.sdk/source/vst/vsteditcontroller.h>

namespace sonic_vst {

class ParameterChangeNotifier : public Steinberg::IDependent {
private:
  Steinberg::Vst::EditController *controller = nullptr;
  std::function<void(Steinberg::Vst::ParamID paramId, double value)> receiver;

public:
  void start(Steinberg::Vst::EditController *controller,
             std::function<void(Steinberg::Vst::ParamID paramId, double value)>
                 receiver);

  void stop();

  void PLUGIN_API update(FUnknown *changedUnknown, int32_t message) override;

  Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID _iid,
                                               void **obj) override {
    QUERY_INTERFACE(_iid, obj, IDependent::iid, IDependent)
    QUERY_INTERFACE(_iid, obj, FUnknown::iid, IDependent)
    return Steinberg::kNoInterface;
  }
  uint32_t PLUGIN_API addRef() override { return 1; }
  uint32_t PLUGIN_API release() override { return 1; }
};

} // namespace sonic_vst