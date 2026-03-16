#pragma once
#include "../../../common/mac-web-view.h"
#include "../../../domain/webview-bridge.h"
#include <public.sdk/source/vst/vsteditcontroller.h>

namespace vst_basis {

using namespace sonic;
using namespace Steinberg;

class PluginEditorView : public Vst::EditorView {
private:
  MacWebView webView;
  ViewRect viewRect{0, 0, 900, 600};
  std::unique_ptr<WebViewBridge> webViewBridge;

public:
  PluginEditorView(Vst::EditController *controller,
                   IControllerFacade &controllerFacade,
                   std::string editorPageUrl)
      : Vst::EditorView(controller) {
    printf("WebViewEditorView::WebViewEditorView\n");

    webView.loadUrl(editorPageUrl);
    auto webViewIo = static_cast<IWebViewIo *>(&webView);
    webViewBridge = std::unique_ptr<WebViewBridge>(
        WebViewBridge::create(controllerFacade, *webViewIo));
    webViewBridge->setup();
  }

  ~PluginEditorView() override {
    printf("WebViewEditorView::~WebViewEditorView\n");
    if (webViewBridge) {
      webViewBridge->teardown();
      webViewBridge.reset();
    }
    webView.removeFromParent();
  }

  tresult PLUGIN_API isPlatformTypeSupported(FIDString type) override {
    printf("WebViewEditorView::isPlatformTypeSupported: %s\n", type);
    if (FIDStringsEqual(type, kPlatformTypeNSView)) {
      return kResultTrue;
    }
    return kResultFalse;
  }
  tresult PLUGIN_API attached(void *parent, FIDString type) override {
    printf("WebViewEditorView::attached: %p, %s\n", parent, type);
    if (FIDStringsEqual(type, kPlatformTypeNSView)) {
      webView.attachToParent(parent);
      webView.setFrame(viewRect.left, viewRect.top,
                       viewRect.right - viewRect.left,
                       viewRect.bottom - viewRect.top);
      return kResultOk;
    }
    return kResultFalse;
  }

  tresult PLUGIN_API canResize() override { return kResultTrue; }

  tresult PLUGIN_API getSize(ViewRect *size) override {
    if (!size) {
      return kInvalidArgument;
    }
    *size = viewRect;
    printf("WebViewEditorView::getSize: %d, %d, %d, %d\n", size->left,
           size->top, size->right, size->bottom);
    return kResultOk;
  }
  tresult PLUGIN_API removed() override {
    printf("WebViewEditorView::removed\n");
    webView.removeFromParent();
    return kResultOk;
  }

  tresult PLUGIN_API onSize(ViewRect *newSize) override {
    if (!newSize) {
      return kInvalidArgument;
    }
    printf("WebViewEditorView::onSize: %d, %d, %d, %d\n", newSize->left,
           newSize->top, newSize->right, newSize->bottom);
    viewRect = *newSize;
    webView.setFrame(newSize->left, newSize->top,
                     newSize->right - newSize->left,
                     newSize->bottom - newSize->top);
    return kResultOk;
  }
};
} // namespace vst_basis