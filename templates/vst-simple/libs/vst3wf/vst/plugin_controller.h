#pragma once

#include "vst3wf/general/logger.h"
#include "vst3wf/modules/event_hub.h"
#include "vst3wf/modules/parameters_manager.h"
#include "vst3wf/synthesizer_base.h"
#include "vst3wf/vst_entry/vst_entry_wrapper.h"
#include <public.sdk/source/vst/vsteditcontroller.h>

namespace vst3wf_plugin {

using namespace vst3wf;

class PluginController : public Steinberg::Vst::EditControllerEx1 {
  using tresult = Steinberg::tresult;

private:
  SynthesizerBase *synthInstance;
  ParametersManager parametersManager;
  ParameterDefinitionsProvider parameterDefinitionsProvider;
  EventHub eventHub;

public:
  PluginController()
      : parametersManager(*this, this->parameters,
                          this->parameterDefinitionsProvider),
        eventHub(*this) {
    logger.start();
    synthInstance = gPluginFactoryGlobalHolder.synthInstantiateFn();
  }
  ~PluginController() SMTG_OVERRIDE {
    delete synthInstance;
    logger.stop();
  }

  static Steinberg::FUnknown *createInstance(void *) {
    return (Steinberg::Vst::IEditController *)new PluginController;
  }

  tresult PLUGIN_API initialize(Steinberg::FUnknown *context) SMTG_OVERRIDE;
  tresult PLUGIN_API terminate() SMTG_OVERRIDE;

  tresult PLUGIN_API setComponentState(Steinberg::IBStream *state)
      SMTG_OVERRIDE;
  Steinberg::IPlugView *PLUGIN_API createView(Steinberg::FIDString name)
      SMTG_OVERRIDE;
  tresult PLUGIN_API setState(Steinberg::IBStream *state) SMTG_OVERRIDE;
  tresult PLUGIN_API getState(Steinberg::IBStream *state) SMTG_OVERRIDE;
  tresult PLUGIN_API setParamNormalized(Steinberg::Vst::ParamID tag,
                                        Steinberg::Vst::ParamValue value)
      SMTG_OVERRIDE;
  tresult PLUGIN_API getParamStringByValue(
      Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue valueNormalized,
      Steinberg::Vst::String128 string) SMTG_OVERRIDE;
  tresult PLUGIN_API getParamValueByString(
      Steinberg::Vst::ParamID tag, Steinberg::Vst::TChar *string,
      Steinberg::Vst::ParamValue &valueNormalized) SMTG_OVERRIDE;

  tresult PLUGIN_API notify(Steinberg::Vst::IMessage *message) SMTG_OVERRIDE;

  DEFINE_INTERFACES
  END_DEFINE_INTERFACES(EditController)
  DELEGATE_REFCOUNT(EditController)

protected:
};

} // namespace vst3wf_plugin
