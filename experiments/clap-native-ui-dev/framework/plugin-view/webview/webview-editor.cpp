#include "./webview-editor.h"
#include "../../core/editor-factory-registry.h"
#include "./mac-web-view.h"
#include "./webview-bridge.h"

namespace sonic {

class WebviewEditorInstance : public IEditorInstance {
  IControllerFacade &controllerFacade;
  sonic::MacWebView webView;
  std::unique_ptr<WebViewBridge> webViewBridge;

public:
  WebviewEditorInstance(IControllerFacade &controllerFacade)
      : controllerFacade(controllerFacade) {}

  virtual void setup(std::string loadTargetSpec) override {
    auto url = loadTargetSpec;
    webView.loadUrl(url);
    auto webViewIo = (IWebViewIo *)&webView;
    webViewBridge = std::unique_ptr<WebViewBridge>(
        WebViewBridge::create(controllerFacade, *webViewIo));
    webViewBridge->setup();
  }

  virtual void teardown() override {
    if (webViewBridge) {
      webViewBridge->teardown();
      webViewBridge.reset();
    }
    webView.setMessageReceiver(nullptr);
    webView.removeFromParent();
  }

  virtual void attachToParent(void *parent) override {
    webView.attachToParent(parent);
  }
  virtual void removeFromParent() override { webView.removeFromParent(); }

  virtual void setFrame(int x, int y, int width, int height) override {
    webView.setFrame(x, y, width, height);
  }
};

void registerWebviewEditorFactory() {
  static bool registered = []() {
    EditorFactoryFn factoryFn = [](IControllerFacade &controllerFacade)
        -> std::unique_ptr<IEditorInstance> {
      return std::make_unique<WebviewEditorInstance>(controllerFacade);
    };
    EditorFactoryRegistry::getInstance()->registerEditorVariant("webview",
                                                                factoryFn);
    printf("webview editor factory registered\n");
    return true;
  }();
}

} // namespace sonic