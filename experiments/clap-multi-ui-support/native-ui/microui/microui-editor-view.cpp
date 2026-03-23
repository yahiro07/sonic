#include "./microui-editor-view.h"

namespace project1_gui_microui {

using namespace sonic_plugin_view_microui;

void MicrouiEditorView::setup() {
  mu_init(&context);
  binder.bindInt(parameters.oscEnabled, "oscEnabled");
  binder.bindFloat(parameters.oscWave, "oscWave");
  binder.bindFloat(parameters.oscPitch, "oscPitch");
  binder.bindFloat(parameters.oscVolume, "oscVolume");
  viewAdaptor.setup([this]() { render(); });
  render();
}

void MicrouiEditorView::teardown() { viewAdaptor.teardown(); }

void MicrouiEditorView::render() {
  auto &screen = window.screen();
  screen.beginFrame();
  screen.clear(Color{51, 51, 51, 255});

  mu_begin(&context);

  if (mu_begin_window(&context, "Window", mu_rect(50, 50, 220, 170))) {

    mu_label(&context, "Hello microui");

    if (mu_button(&context, "Note(60)")) {
      noteState = !noteState;
      if (noteState) {
        controllerFacade.requestNoteOn(60, 1.0f);
      } else {
        controllerFacade.requestNoteOff(60);
      }
    }

    mu_checkbox(&context, "Enabled", &parameters.oscEnabled);
    mu_slider_ex(&context, &parameters.oscWave, 0.0f, 3.0f, 0, "%.2f",
                 MU_OPT_ALIGNCENTER);

    mu_slider_ex(&context, &parameters.oscPitch, 0.0f, 1.0f, 0, "%.2f",
                 MU_OPT_ALIGNCENTER);
    mu_slider_ex(&context, &parameters.oscVolume, 0.0f, 1.0f, 0, "%.2f",
                 MU_OPT_ALIGNCENTER);
    mu_end_window(&context);
  }

  mu_end(&context);
  EditorHelper::sendMicroUiCommandsToScreen(context, screen);

  binder.sync();
}

} // namespace project1_gui_microui
