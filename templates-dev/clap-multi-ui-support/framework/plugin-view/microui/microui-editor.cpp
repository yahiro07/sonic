#include "../../core/editor-factory-registry.h"
#include "./editor-view.h"
#include "./window-representor.h"

namespace sonic_plugin_view_microui {

using namespace sonic;

class MicrouiEditorInstance : public IEditorInstance {
  IControllerFacade &controllerFacade;
  WindowRepresentor window;
  std::unique_ptr<IEditor> editor;

public:
  MicrouiEditorInstance(IControllerFacade &controllerFacade)
      : controllerFacade(controllerFacade) {
    editor = sonic_plugin_view_microui::createEditor(window);
  }

  void setup(std::string /*loadTargetSpec*/) override { editor->setup(); }

  void teardown() override {
    editor->dispose();
    window.removeFromParent();
  }

  void attachToParent(void *parent) override { window.attachToParent(parent); }
  void removeFromParent() override { window.removeFromParent(); }

  void setFrame(int x, int y, int width, int height) override {
    window.setFrame(x, y, width, height);
  }
};

} // namespace sonic_plugin_view_microui

namespace sonic {

void registerMicrouiEditorFactory() {
  static bool registered = []() {
    EditorFactoryFn factoryFn = [](IControllerFacade &controllerFacade)
        -> std::unique_ptr<IEditorInstance> {
      return std::make_unique<sonic_plugin_view_microui::MicrouiEditorInstance>(
          controllerFacade);
    };
    EditorFactoryRegistry::getInstance()->registerEditorVariant("microui",
                                                                factoryFn);
    printf("microui editor factory registered\n");
    return true;
  }();
}

} // namespace sonic