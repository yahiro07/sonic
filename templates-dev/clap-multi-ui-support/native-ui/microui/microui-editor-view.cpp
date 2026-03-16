#include "./microui-editor-view.h"
#include "./microui-view-adapter.h"

namespace project1_gui {

class EditorView final : public sonic_plugin_view_microui::IMicrouiEditor {
private:
  MicrouiViewAdaptor viewAdaptor;
  IWindowRepresentor &window;
  sonic::IControllerFacade &controllerFacade;
  mu_Context context{};
  float sliderValue = 0.5f;
  float sliderValue2 = 0.5f;

public:
  explicit EditorView(IWindowRepresentor &window,
                      sonic::IControllerFacade &controllerFacade)
      : window(window), controllerFacade(controllerFacade),
        viewAdaptor(window, context) {
    mu_init(&context);
    // for (const auto &paramItem : controllerFacade.getParameterSpecs()) {
    //   printf("parameter id: %u, key: %s\n", paramItem.id,
    //          paramItem.paramKey.c_str());
    // }
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

      mu_slider_ex(&context, &sliderValue, 0.0f, 1.0f, 0, "%.2f",
                   MU_OPT_ALIGNCENTER);
      mu_slider_ex(&context, &sliderValue2, 0.0f, 1.0f, 0, "%.2f",
                   MU_OPT_ALIGNCENTER);
      mu_label(&context, "Hello microui");
      mu_end_window(&context);
    }

    mu_end(&context);
    EditorHelper::sendMicroUiCommandsToScreen(context, screen);
  }
};

sonic_plugin_view_microui::IMicrouiEditor *
createMicrouiEditor(sonic_plugin_view_microui::IWindowRepresentor &window,
                    sonic::IControllerFacade &controllerFacade) {
  return new EditorView(window, controllerFacade);
}

} // namespace project1_gui
