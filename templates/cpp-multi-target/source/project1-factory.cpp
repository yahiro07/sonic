
#include "project1-synthesizer.h"
#include <sonic/common/logger.h>
#include <sonic/plugin-view/webview/webview-editor.h>
#include <stdio.h>

sonic::SynthesizerBase *createSynthesizerInstance() {
  sonic::logger.trace("----------------------------------------");
  sonic::logger.info("createSynthesizerInstance");
  sonic::registerWebviewEditorFactory();
  return new project1::Project1Synthesizer();
}