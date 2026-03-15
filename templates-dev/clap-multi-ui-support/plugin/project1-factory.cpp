
#include "../framework/plugin-view/microui/microui-editor.h"
#include "../framework/plugin-view/webview/webview-editor.h"
#include "project1-synthesizer.h"
#include <stdio.h>

sonic::SynthesizerBase *createSynthesizerInstance() {
  printf("project1-synthesizer: createSynthesizerInstance 0655\n");
  sonic::registerWebviewEditorFactory();
  sonic::registerMicrouiEditorFactory();
  return new project1::Project1Synthesizer();
}