#pragma once
#include "../../core/editor-interfaces.h"
#include "./window-representor.h"

namespace sonic_plugin_view_microui {

class IMicrouiEditor {
public:
  virtual ~IMicrouiEditor() = default;
  virtual void setup() = 0;
  virtual void teardown() = 0;
};

typedef IMicrouiEditor *(*MicrouiEditorFactoryFn)(
    IWindowRepresentor &window, sonic::IControllerFacade &controllerFacade);

void registerMicrouiEditorFactory(
    std::string name,
    sonic_plugin_view_microui::MicrouiEditorFactoryFn factoryFn);

} // namespace sonic_plugin_view_microui
