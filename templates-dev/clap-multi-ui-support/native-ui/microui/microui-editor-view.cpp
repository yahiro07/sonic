#include "./microui-editor-view.h"
#include "../../framework/plugin-view/microui/microui-view-adapter.h"
#include "../../framework/plugin-view/microui/parameter-binding-helper.h"

namespace project1_gui {

using namespace sonic_plugin_view_microui;

struct Parameters {
  int oscEnabled;
  float oscWave;
  float oscPitch;
  float oscVolume;
};

class EditorView : public IMicrouiEditor {
private:
  IWindowRepresentor &window;
  mu_Context context{};
  MicrouiViewAdaptor viewAdaptor{window, context};
  sonic::IControllerFacade &controllerFacade;
  ParameterBindingHelper binder{controllerFacade};

  Parameters parameters;

public:
  explicit EditorView(IWindowRepresentor &window,
                      sonic::IControllerFacade &controllerFacade)
      : window(window), controllerFacade(controllerFacade) {
    mu_init(&context);
    binder.bindInt(parameters.oscEnabled, "oscEnabled");
    binder.bindFloat(parameters.oscWave, "oscWave");
    binder.bindFloat(parameters.oscPitch, "oscPitch");
    binder.bindFloat(parameters.oscVolume, "oscVolume");
  }

  ~EditorView() override { teardown(); }

  void setup() override {
    viewAdaptor.setup([this]() { render(); });
    render();
  }

  void teardown() override { viewAdaptor.teardown(); }

  void render() {
    auto &screen = window.screen();
    screen.beginFrame();
    screen.clear(Color{51, 51, 51, 255});

    mu_begin(&context);

    if (mu_begin_window(&context, "Window", mu_rect(50, 50, 220, 170))) {
      if (mu_button(&context, "Button")) {
      }

      if (mu_button(&context, "Button2")) {
      }

      mu_checkbox(&context, "Checkbox", &parameters.oscEnabled);
      mu_slider_ex(&context, &parameters.oscWave, 0.0f, 3.0f, 0, "%.2f",
                   MU_OPT_ALIGNCENTER);

      mu_slider_ex(&context, &parameters.oscPitch, 0.0f, 1.0f, 0, "%.2f",
                   MU_OPT_ALIGNCENTER);
      mu_slider_ex(&context, &parameters.oscVolume, 0.0f, 1.0f, 0, "%.2f",
                   MU_OPT_ALIGNCENTER);
      mu_label(&context, "Hello microui");
      mu_end_window(&context);
    }

    mu_end(&context);
    EditorHelper::sendMicroUiCommandsToScreen(context, screen);

    binder.sync();
  }
};

sonic_plugin_view_microui::IMicrouiEditor *
createMicrouiEditor(sonic_plugin_view_microui::IWindowRepresentor &window,
                    sonic::IControllerFacade &controllerFacade) {
  return new EditorView(window, controllerFacade);
}

} // namespace project1_gui
