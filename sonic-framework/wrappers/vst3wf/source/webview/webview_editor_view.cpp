#include "edit_controller_parameter_change_notifier.h"
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/gui/iplugview.h"
#include <public.sdk/source/vst/vsteditcontroller.h>

#include "mac_web_view.h"

using namespace Steinberg;

#include "../project1_cids.h"
#include "../utils/logger.h"

class WebViewMessagingHub {
private:
  EditControllerParameterChangeNotifier paramChangeNotifier;
  IWebViewIo *webView;
  Vst::EditController *controller;

public:
  void start(Vst::EditController *controller, IWebViewIo *webViewIo) {
    this->controller = controller;
    this->webView = webViewIo;
    webView->setMessageReceiver([this](const std::string &message) {
      logger.log("message received: %s", message.c_str());
      auto value = message == "ON" ? 1.0 : 0.0;
      this->controller->setParamNormalized(Project1Params::kParamOnId, value);
      this->controller->beginEdit(Project1Params::kParamOnId);
      this->controller->performEdit(Project1Params::kParamOnId, value);
      this->controller->endEdit(Project1Params::kParamOnId);
    });
    paramChangeNotifier.start(controller, [this](int paramId, double value) {
      printf("WebViewEditorView::paramChangeNotifier: %d, %f\n", paramId,
             value);
      if (paramId == Project1Params::kParamOnId) {
        this->webView->sendMessage(value == 1.0 ? "'ON'" : "'OFF'");
      }
    });
  }
  void stop() {
    paramChangeNotifier.stop();
    webView->setMessageReceiver(nullptr);
  }
};

class WebViewEditorView : public Vst::EditorView {
private:
  MacWebView webView;
  ViewRect viewRect{0, 0, 900, 600};
  WebViewMessagingHub webViewMessagingHub;

public:
  IWebViewIo *getWebViewIO() { return &webView; }

  WebViewEditorView(Vst::EditController *controller)
      : Vst::EditorView(controller) {
    printf("WebViewEditorView::WebViewEditorView\n");

    // webView.loadUrl("https://localhost:3002");
    webView.loadUrl("app://local/index.html");
    webViewMessagingHub.start(controller, &webView);
  }

  ~WebViewEditorView() override {
    printf("WebViewEditorView::~WebViewEditorView\n");
    webViewMessagingHub.stop();
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

Steinberg::IPlugView *
createWebViewEditorView(Steinberg::Vst::EditController *controller) {
  return new WebViewEditorView(controller);
}