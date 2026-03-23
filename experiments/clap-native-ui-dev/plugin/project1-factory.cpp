
#include "../native-ui/briq/briq-editor-view.h"
#include "../native-ui/microui/microui-editor-view.h"
#include "project1-synthesizer.h"
#include <sonic/plugin-view/microui/microui-editor.h>
#include <sonic/plugin-view/webview/webview-editor.h>

sonic::SynthesizerBase *createSynthesizerInstance() {
  printf("project1-synthesizer: createSynthesizerInstance 0655\n");
  sonic::registerWebviewEditorFactory();
  sonic_plugin_view_microui::registerMicrouiEditorFactory(
      "default", project1_gui::createMicrouiEditor);

  sonic_plugin_view_briq::registerBriqEditorFactory(
      "default", [](briq::EditorIntegration &editorIntegration,
                    sonic::IControllerFacade &controllerFacade) {
        return new project1_gui_briq::BriqEditorView(editorIntegration,
                                                     controllerFacade);
      });
  return new project1::Project1Synthesizer();
}