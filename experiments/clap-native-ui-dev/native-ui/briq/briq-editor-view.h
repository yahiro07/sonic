#pragma once

#include <briq/editor.h>

#include "../../framework/core/editor-interfaces.h"
#include "../../framework/plugin-view/briq/briq-editor.h"

namespace project1_gui_briq {

class BriqEditorView : public sonic_plugin_view_briq::IBriqEditor {
private:
  briq::EditorIntegration &editorIntegration;
  sonic::IControllerFacade &controllerFacade;

public:
  BriqEditorView(briq::EditorIntegration &editorIntegration,
                 sonic::IControllerFacade &controllerFacade)
      : editorIntegration(editorIntegration),
        controllerFacade(controllerFacade) {}
  ~BriqEditorView() = default;

  void setup() override;
  void teardown() override;
};

} // namespace project1_gui_briq
