#include "briq-editor.h"
#include "../../core/editor-factory-registry.h"

#include <memory>

namespace sonic_plugin_view_briq {

using namespace sonic;

static std::map<std::string, BriqEditorFactoryFn> factoryMap;

class BriqEditorAdapter : public IEditorInstance {
  briq::EditorIntegration editorIntegration;
  IControllerFacade &controllerFacade;
  std::unique_ptr<IBriqEditor> editor;

public:
  BriqEditorAdapter(IControllerFacade &controllerFacade)
      : controllerFacade(controllerFacade) {}

  void setup(std::string loadTargetSpec) override {
    auto factoryFn = factoryMap.at(loadTargetSpec);
    if (!factoryFn) {
      printf("briq editor factory not found for %s\n", loadTargetSpec.c_str());
      return;
    }
    editor = std::unique_ptr<IBriqEditor>(
        std::move(factoryFn(editorIntegration, controllerFacade)));
    editor->setup();
  }

  void teardown() override {
    if (editor) {
      editor->teardown();
      editor.reset();
    }
    editorIntegration.removeFromParent();
  }

  void attachToParent(void *parent) override {
    editorIntegration.attachToParent(parent);
  }
  void removeFromParent() override { editorIntegration.removeFromParent(); }

  void setFrame(int x, int y, int width, int height) override {
    editorIntegration.setBounds(x, y, width, height);
  }
};

void registerBriqEditorFactory(std::string name,
                               BriqEditorFactoryFn factoryFn) {

  factoryMap[name] = factoryFn;

  static bool registered = []() {
    EditorFactoryFn factoryFn = [](IControllerFacade &controllerFacade)
        -> std::unique_ptr<IEditorInstance> {
      return std::make_unique<BriqEditorAdapter>(controllerFacade);
    };
    EditorFactoryRegistry::getInstance()->registerEditorVariant("briq",
                                                                factoryFn);
    printf("briq editor factory registered\n");
    return true;
  }();
}

} // namespace sonic_plugin_view_briq
