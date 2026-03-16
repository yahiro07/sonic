#include "./microui-editor-view.h"
#include "../../framework/plugin-view/microui/microui-view-adapter.h"
#include <optional>

namespace project1_gui {

using namespace sonic_plugin_view_microui;

struct Parameters {
  float oscPitch;
  float oscVolume;
};

struct BindingItem {
  uint32_t paramId;
  float *uiValue;
  float *cachedValue;
};

class ParameterBindingHelper {
private:
  sonic::IControllerFacade &controllerFacade;

  std::map<std::string, float> cachedParameters;

  std::vector<BindingItem> bindingItems;

public:
  explicit ParameterBindingHelper(sonic::IControllerFacade &controllerFacade)
      : controllerFacade(controllerFacade) {
    auto paramDefs = controllerFacade.getParameterSpecs();
    for (const auto &paramDef : paramDefs) {
      cachedParameters[paramDef.paramKey] = paramDef.defaultValue;
    }
  }

  void bindFloat(float *parameterValue, std::string paramKey) {
    auto _paramId = controllerFacade.getParameterIdByParamKey(paramKey);
    if (_paramId == std::nullopt)
      return;
    auto paramId = *_paramId;

    BindingItem bindingItem{
        .paramId = paramId,
        .uiValue = parameterValue,
        .cachedValue = &cachedParameters[paramKey],
    };
    *bindingItem.uiValue = *bindingItem.cachedValue;

    bindingItems.push_back(bindingItem);
  }

  void sync() {
    for (const auto &bindingItem : bindingItems) {
      if (*bindingItem.uiValue != *bindingItem.cachedValue) {
        controllerFacade.applyParameterEditFromUi(
            bindingItem.paramId, *bindingItem.uiValue,
            sonic::ParameterEditState::Perform);
        *bindingItem.cachedValue = *bindingItem.uiValue;
      }
    }
  }
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
    binder.bindFloat(&parameters.oscPitch, "oscPitch");
    binder.bindFloat(&parameters.oscVolume, "oscVolume");
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
