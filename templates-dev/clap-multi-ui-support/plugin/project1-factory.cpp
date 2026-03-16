
#include "../framework/plugin-view/microui/microui-editor.h"
#include "../framework/plugin-view/webview/webview-editor.h"
#include "../native-ui/microui/microui-editor-view.h"
#include "project1-synthesizer.h"
#include <stdio.h>

sonic::SynthesizerBase *createSynthesizerInstance() {
  printf("project1-synthesizer: createSynthesizerInstance 0655\n");
  sonic::registerWebviewEditorFactory();
  sonic_plugin_view_microui::registerMicrouiEditorFactory(
      "default", project1_gui::createMicrouiEditor);
  return new project1::Project1Synthesizer();
}