#pragma once
#include <briq/editor.h>
#include <functional>
#include <sonic/core/editor-interfaces.h>

namespace sonic_plugin_view_briq {

class IBriqEditor {
public:
  virtual ~IBriqEditor() = default;
  virtual void setup() = 0;
  virtual void teardown() = 0;
};

using BriqEditorFactoryFn =
    std::function<IBriqEditor *(briq::EditorIntegration &editorIntegration,
                                sonic::IControllerFacade &controllerFacade)>;

void registerBriqEditorFactory(
    std::string name, sonic_plugin_view_briq::BriqEditorFactoryFn factoryFn);

} // namespace sonic_plugin_view_briq
