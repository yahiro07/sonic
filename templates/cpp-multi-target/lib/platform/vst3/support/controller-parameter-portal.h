#pragma once
#include "../logic/interfaces.h"
#include "../modules/parameter_change_notifier.h"
#include <public.sdk/source/vst/vsteditcontroller.h>

namespace vst_support {
using namespace sonic;
using namespace Steinberg::Vst;
using namespace sonic_vst;

class ControllerParameterPortal : public IControllerParameterPortal {
private:
  EditController &editController;
  ParameterChangeNotifier parameterChangeNotifier;

  std::function<void(uint32_t, double)> listener;
  int editingParamId = -1;

  void setEditing(int paramId) { editingParamId = paramId; }
  void clearEditing() { editingParamId = -1; }

public:
  ControllerParameterPortal(EditController &editController)
      : editController(editController) {}

  void startObserve() {
    parameterChangeNotifier.start(&this->editController,
                                  [&](ParamID id, double normValue) {
                                    if (id == editingParamId) {
                                      return;
                                    }
                                    if (listener) {
                                      listener(id, normValue);
                                    }
                                  });
  }

  void stopObserve() { parameterChangeNotifier.stop(); }

  void
  subscribeParameterChange(std::function<void(uint32_t, double)> fn) override {
    listener = fn;
  }
  void unsubscribeParameterChange() override { listener = nullptr; }

  void applyParameterEdit(uint32_t paramId, double value,
                          ParameterEditState editState) override {
    if (editState == ParameterEditState::Begin) {
      setEditing(paramId);
      editController.beginEdit(editingParamId);
    } else if (editState == ParameterEditState::Perform) {
      editController.performEdit(editingParamId, value);
    } else if (editState == ParameterEditState::End) {
      editController.endEdit(editingParamId);
      clearEditing();
    } else if (editState == ParameterEditState::InstantChange) {
      setEditing(paramId);
      editController.beginEdit(paramId);
      editController.performEdit(paramId, value);
      editController.endEdit(paramId);
      clearEditing();
    }
  }
};

} // namespace vst_support