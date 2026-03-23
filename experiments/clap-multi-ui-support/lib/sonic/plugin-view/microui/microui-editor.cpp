#include "microui-editor.h"
#include "./window-representor.h"
#include <memory>
#include <sonic/core/editor-factory-registry.h>

namespace sonic_plugin_view_microui {

using namespace sonic;

static std::map<std::string, MicrouiEditorFactoryFn> factoryMap;

class MicrouiEditorInstance : public IEditorInstance {
  IControllerFacade &controllerFacade;
  WindowRepresentor window;
  std::unique_ptr<IMicrouiEditor> editor;

public:
  MicrouiEditorInstance(IControllerFacade &controllerFacade)
      : controllerFacade(controllerFacade) {}

  void setup(std::string loadTargetSpec) override {
    auto factoryFn = factoryMap.at(loadTargetSpec);
    if (!factoryFn) {
      printf("microui editor factory not found for %s\n",
             loadTargetSpec.c_str());
      return;
    }
    editor = std::unique_ptr<IMicrouiEditor>(
        std::move(factoryFn(window, controllerFacade)));
    editor->setup();
  }

  void teardown() override {
    if (editor) {
      editor->teardown();
      editor.reset();
    }
    window.removeFromParent();
  }

  void attachToParent(void *parent) override { window.attachToParent(parent); }
  void removeFromParent() override { window.removeFromParent(); }

  void setFrame(int x, int y, int width, int height) override {
    window.setFrame(x, y, width, height);
  }
};

void registerMicrouiEditorFactory(
    std::string name,
    sonic_plugin_view_microui::MicrouiEditorFactoryFn factoryFn) {

  factoryMap[name] = factoryFn;

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

} // namespace sonic_plugin_view_microui
