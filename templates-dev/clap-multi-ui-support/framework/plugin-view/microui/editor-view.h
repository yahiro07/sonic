#pragma once

#include <memory>

#include "window-representor.h"

namespace sonic_plugin_view_microui {

class IEditor {
public:
  virtual ~IEditor() = default;
  virtual void setup() = 0;
  virtual void dispose() = 0;
};

std::unique_ptr<IEditor> createEditor(IWindowRepresentor &window);

} // namespace sonic_plugin_view_microui
