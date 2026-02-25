#pragma once
#include "./parameters_manager.h"
#include <public.sdk/source/vst/vsteditcontroller.h>

namespace vst3wf {

Steinberg::IPlugView *
createWebViewEditorView(Steinberg::Vst::EditController *controller,
                        ParametersManager *parametersManager);

}