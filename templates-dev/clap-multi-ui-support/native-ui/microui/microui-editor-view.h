#pragma once

#include "../../framework/core/editor-interfaces.h"
#include "../../framework/plugin-view/microui/microui-editor.h"
#include "../../framework/plugin-view/microui/window-representor.h"

namespace project1_gui {

sonic_plugin_view_microui::IMicrouiEditor *
createMicrouiEditor(sonic_plugin_view_microui::IWindowRepresentor &window,
                    sonic::IControllerFacade &controllerFacade);

} // namespace project1_gui
