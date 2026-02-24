//------------------------------------------------------------------------
// Copyright(c) 2022 Steinberg Media Technologies GmbH.
//------------------------------------------------------------------------

#pragma once

#include "dsp/SynthesizerBase.h"
#include "public.sdk/source/vst/vsteditcontroller.h"

#include "dsp/MySynthesizer.h"
#include "utils/logger.h"

#include "parameters/parameters_manager.h"

namespace Steinberg {

//------------------------------------------------------------------------
//  Project1Controller
//------------------------------------------------------------------------
class Project1Controller : public Steinberg::Vst::EditControllerEx1 {
private:
  SynthesizerBase *synthInstance;
  ParametersManager parametersManager;

public:
  //------------------------------------------------------------------------
  Project1Controller() : parametersManager(this->parameters) {
    logger.start();
    synthInstance = createSynthesizerInstance();
  }
  ~Project1Controller() SMTG_OVERRIDE {
    delete synthInstance;
    logger.stop();
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
} // namespace Steinberg
