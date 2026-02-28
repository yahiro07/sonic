#pragma once
#include "./event_hub.h"
#include "./parameters_manager.h"
#include <public.sdk/source/vst/vsteditcontroller.h>

namespace sonic_vst {

Steinberg::IPlugView *
createWebViewEditorView(Steinberg::Vst::EditController *controller,
                        ParametersManager *parametersManager,
                        EventHub *eventHub, std::string editorPageUrl);

}