
#include "project1-synthesizer.h"
#include <sonic/plugin-view/webview/webview-editor.h>
#include <stdio.h>

sonic::SynthesizerBase *createSynthesizerInstance() {
  printf("project1-synthesizer: createSynthesizerInstance 1434\n");
  sonic::registerWebviewEditorFactory();
  return new project1::Project1Synthesizer();
}