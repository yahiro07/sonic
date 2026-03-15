#pragma once

#include <memory>

#include "../../core/editor-interfaces.h"
#include "window-representor.h"

namespace sonic_plugin_view_microui {

class IEditor {
public:
  virtual ~IEditor() = default;
  virtual void setup() = 0;
  virtual void teardown() = 0;
};

std::unique_ptr<IEditor>
createEditor(IWindowRepresentor &window,
             sonic::IControllerFacade &controllerFacade);

} // namespace sonic_plugin_view_microui
