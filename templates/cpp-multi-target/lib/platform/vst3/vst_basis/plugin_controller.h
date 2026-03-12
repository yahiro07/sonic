#pragma once

#include "../../../common/logger.h"
#include "../modules/event_hub.h"
#include "../modules/parameters_manager.h"
#include "../vst_entry/vst_entry_wrapper.h"
#include <public.sdk/source/vst/vsteditcontroller.h>

namespace vst_basis {

using namespace sonic_vst;
using namespace Steinberg;

class PluginController : public Vst::EditControllerEx1 {
private:
  SynthesizerBase *synthInstance;
  ParametersManager parametersManager;
  ParameterRegistry parameterRegistry;
  EventHub eventHub;

public:
  PluginController()
      : parametersManager(*this, this->parameters, this->parameterRegistry),
        eventHub(*this) {
    logger.start();
    synthInstance = gPluginFactoryGlobalHolder.synthInstantiateFn();
  }
  ~PluginController() SMTG_OVERRIDE {
    delete synthInstance;
    logger.stop();
  }
  static FUnknown *createInstance(void *) {
    return (Vst::IEditController *)new PluginController;
  }

  tresult PLUGIN_API initialize(FUnknown *context) SMTG_OVERRIDE;
  tresult PLUGIN_API terminate() SMTG_OVERRIDE;

  tresult PLUGIN_API setComponentState(IBStream *state) SMTG_OVERRIDE;
  IPlugView *PLUGIN_API createView(FIDString name) SMTG_OVERRIDE;
  tresult PLUGIN_API setState(IBStream *state) SMTG_OVERRIDE;
  tresult PLUGIN_API getState(IBStream *state) SMTG_OVERRIDE;
  tresult PLUGIN_API setParamNormalized(Vst::ParamID tag,
                                        Vst::ParamValue value) SMTG_OVERRIDE;
  tresult PLUGIN_API getParamStringByValue(Vst::ParamID tag,
                                           Vst::ParamValue valueNormalized,
                                           Vst::String128 string) SMTG_OVERRIDE;
  tresult PLUGIN_API getParamValueByString(Vst::ParamID tag, Vst::TChar *string,
                                           Vst::ParamValue &valueNormalized)
      SMTG_OVERRIDE;

  tresult PLUGIN_API notify(Vst::IMessage *message) SMTG_OVERRIDE;

  DEFINE_INTERFACES
  END_DEFINE_INTERFACES(EditController)
  DELEGATE_REFCOUNT(EditController)

protected:
};

} // namespace vst_basis
