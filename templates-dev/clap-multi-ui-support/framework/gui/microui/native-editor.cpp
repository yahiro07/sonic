#include "../../core/editor-factory-registry.h"

namespace sonic {

class NativeEditorInstance : public IEditorInstance {
  IControllerFacade &controllerFacade;

public:
  NativeEditorInstance(IControllerFacade &controllerFacade)
      : controllerFacade(controllerFacade) {}

  virtual void setup(std::string loadTargetSpec) override {}

  virtual void teardown() override {}

  virtual void attachToParent(void *parent) override {}
  virtual void removeFromParent() override {}

  virtual void setFrame(int x, int y, int width, int height) override {}
};

void registerNativeEditorFactory() {
  static bool registered = false;
  if (registered) {
    return;
  }
  registered = true;

  EditorFactoryFn factoryFn = [](IControllerFacade &controllerFacade)
      -> std::unique_ptr<IEditorInstance> {
    return std::make_unique<NativeEditorInstance>(controllerFacade);
  };
  EditorFactoryRegistry::getInstance()->registerEditorVariant("native",
                                                              factoryFn);
  printf("native editor factory registered\n");
}

} // namespace sonic