#include "./microui-editor-view.h"
#include "../../framework/plugin-view/microui/microui-view-adapter.h"
#include <cmath>
#include <optional>

namespace project1_gui {

using namespace sonic_plugin_view_microui;

struct Parameters {
  int oscEnabled;
  float oscWave;
  float oscPitch;
  float oscVolume;
};

struct BindingItem {
  uint32_t paramId;
  std::function<float()> getUiValue;
  float cachedValue;
};

class ParameterBindingHelper {
private:
  sonic::IControllerFacade &controllerFacade;

  std::map<std::string, float> initialParameters;

  std::vector<BindingItem> bindingItems;

  void bindParamFn(std::string paramKey, std::function<float()> getUiValue) {
    auto _paramId = controllerFacade.getParameterIdByParamKey(paramKey);
    if (_paramId == std::nullopt)
      return;
    auto paramId = *_paramId;

    BindingItem bindingItem{
        .paramId = paramId,
        .getUiValue = getUiValue,
        .cachedValue = getUiValue(),
    };

    bindingItems.push_back(bindingItem);
  }

public:
  explicit ParameterBindingHelper(sonic::IControllerFacade &controllerFacade)
      : controllerFacade(controllerFacade) {
    auto paramDefs = controllerFacade.getParameterSpecs();
    for (const auto &paramDef : paramDefs) {
      initialParameters[paramDef.paramKey] = paramDef.defaultValue;
    }
  }

  void bindFloat(float &value, std::string paramKey) {
    value = initialParameters[paramKey];
    bindParamFn(paramKey, [&value]() { return value; });
  }

  void bindInt(int &value, std::string paramKey) {
    value = (int)roundf(initialParameters[paramKey]);
    bindParamFn(paramKey, [&value]() { return (float)value; });
  }

  void sync() {
    for (auto &bindingItem : bindingItems) {
      auto newValue = bindingItem.getUiValue();
      if (newValue != bindingItem.cachedValue) {
        controllerFacade.applyParameterEditFromUi(
            bindingItem.paramId, newValue, sonic::ParameterEditState::Perform);
        bindingItem.cachedValue = newValue;
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
