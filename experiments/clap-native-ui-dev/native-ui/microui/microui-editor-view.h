#pragma once

#include <sonic/core/editor-interfaces.h>
#include <sonic/plugin-view/microui/microui-editor.h>
#include <sonic/plugin-view/microui/microui-view-adapter.h>
#include <sonic/plugin-view/microui/parameter-binding-helper.h>

namespace project1_gui_microui {

struct Parameters {
  int oscEnabled;
  float oscWave;
  float oscPitch;
  float oscVolume;
};

class MicrouiEditorView : public sonic_plugin_view_microui::IMicrouiEditor {
private:
  sonic_plugin_view_microui::IWindowRepresentor &window;
  sonic::IControllerFacade &controllerFacade;
  mu_Context context{};
  sonic_plugin_view_microui::MicrouiViewAdaptor viewAdaptor{window, context};
  sonic_plugin_view_microui::ParameterBindingHelper binder{controllerFacade};

  bool noteState = false;
  Parameters parameters;

public:
  MicrouiEditorView(sonic_plugin_view_microui::IWindowRepresentor &window,
                    sonic::IControllerFacade &controllerFacade)
      : window(window), controllerFacade(controllerFacade) {}

  ~MicrouiEditorView() override { teardown(); }

  void setup() override;

  void teardown() override;

private:
  void render();
};

} // namespace project1_gui_microui
