//------------------------------------------------------------------------
// Copyright(c) 2022 Steinberg Media Technologies GmbH.
//------------------------------------------------------------------------

#pragma once

#include "vst3wf/SynthesizerBase.h"
#include "vst3wf/general/logger.h"
#include "vst3wf/modules/parameters_manager.h"
#include "vst3wf/vst_entry/vst_entry_wrapper.h"
#include <public.sdk/source/vst/vsteditcontroller.h>

namespace Project1 {

//------------------------------------------------------------------------
//  Project1Controller
//------------------------------------------------------------------------
class Project1Controller : public Steinberg::Vst::EditControllerEx1 {
private:
  SynthesizerBase *synthInstance;
  vst3wf::ParametersManager parametersManager;

public:
  //------------------------------------------------------------------------
  Project1Controller() : parametersManager(*this, this->parameters) {
    vst3wf::logger.start();
    synthInstance = vst3wf::gPluginFactoryGlobalHolder.synthInstantiateFn();
  }
  ~Project1Controller() SMTG_OVERRIDE {
    delete synthInstance;
    vst3wf::logger.stop();
  }

  // Create function
  static Steinberg::FUnknown *createInstance(void * /*context*/) {
    return (Steinberg::Vst::IEditController *)new Project1Controller;
  }

  // IPluginBase
  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown *context)
      SMTG_OVERRIDE;
  Steinberg::tresult PLUGIN_API terminate() SMTG_OVERRIDE;

  // EditController
  Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream *state)
      SMTG_OVERRIDE;
  Steinberg::IPlugView *PLUGIN_API createView(Steinberg::FIDString name)
      SMTG_OVERRIDE;
  Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream *state)
      SMTG_OVERRIDE;
  Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream *state)
      SMTG_OVERRIDE;
  Steinberg::tresult PLUGIN_API
  setParamNormalized(Steinberg::Vst::ParamID tag,
                     Steinberg::Vst::ParamValue value) SMTG_OVERRIDE;
  Steinberg::tresult PLUGIN_API getParamStringByValue(
      Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue valueNormalized,
      Steinberg::Vst::String128 string) SMTG_OVERRIDE;
  Steinberg::tresult PLUGIN_API getParamValueByString(
      Steinberg::Vst::ParamID tag, Steinberg::Vst::TChar *string,
      Steinberg::Vst::ParamValue &valueNormalized) SMTG_OVERRIDE;

  //---Interface---------
  DEFINE_INTERFACES
  // Here you can add more supported VST3 interfaces
  // DEF_INTERFACE (Vst::IXXX)
  END_DEFINE_INTERFACES(EditController)
  DELEGATE_REFCOUNT(EditController)

  //------------------------------------------------------------------------
protected:
};

//------------------------------------------------------------------------
} // namespace Project1
