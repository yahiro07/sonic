#pragma once
#include "./parameters_manager.h"
#include <public.sdk/source/vst/vsteditcontroller.h>

Steinberg::IPlugView *
createWebViewEditorView(Steinberg::Vst::EditController *controller,
                        ParametersManager *parametersManager);