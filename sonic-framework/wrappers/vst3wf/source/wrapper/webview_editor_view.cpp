#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/gui/iplugview.h"
#include <public.sdk/source/vst/vsteditcontroller.h>

#include "../general/mac_web_view.h"
#include "./parameters_manager.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include <glaze/glaze.hpp>

using namespace Steinberg;
#include "../general/logger.h"

struct MsgSetParameter {
  std::string type = "setParameter";
  std::string identifier;
  float value;
};
struct MsgUiLoaded {
  std::string type = "uiLoaded";
};

using MessageFromUI = std::variant<MsgSetParameter, MsgUiLoaded>;

std::optional<MessageFromUI>
mapMessageFromUI_fromString(const std::string &jsonStr) {
  MessageFromUI msg;
  if (glz::read_json(msg, jsonStr)) {
    logger.log(
        "messageFromUI_fromJson: %s",
        glz::format_error(glz::read_json(msg, jsonStr), jsonStr).c_str());
    return std::nullopt;
  }
  return msg;
};

class WebViewMessagingHub {
private:
  IWebViewIo *webView;
  ParametersManager *parametersManager;
  int parametersManagerSubscriptionId = 0;

public:
  void start(ParametersManager *parametersManager, IWebViewIo *webViewIo) {
    this->parametersManager = parametersManager;
    this->webView = webViewIo;
    webView->setMessageReceiver([this](const std::string &jsonStr) {
      logger.log("message received: %s", jsonStr.c_str());

      auto msg = mapMessageFromUI_fromString(jsonStr);
      if (!msg)
        return;
      if (auto *p = std::get_if<MsgSetParameter>(&*msg)) {
        auto identifier = p->identifier;
        auto value = p->value;
        logger.log("set parameter: %s, %f", identifier.c_str(), value);
        this->parametersManager->applyParameterEdit(
            identifier, value, ParameterEditingState::InstantChange);
      } else if (auto *p = std::get_if<MsgUiLoaded>(&*msg)) {
        logger.log("ui loaded");
      }
    });
    parametersManagerSubscriptionId = parametersManager->subscribeFromEditor(
        [this](std::string identifier, double value) {
          printf("WebViewEditorView::subscribeFromEditor: %s, %f\n",
                 identifier.c_str(), value);
          // this->webView->sendMessage(
          //     std::format("{} {}", identifier, value).c_str());
        });
  }
  void stop() {
    if (parametersManagerSubscriptionId > 0) {
      parametersManager->unsubscribeFromEditor(parametersManagerSubscriptionId);
    }
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

  WebViewEditorView(Vst::EditController *controller,
                    ParametersManager *parametersManager)
      : Vst::EditorView(controller) {
    printf("WebViewEditorView::WebViewEditorView\n");

    // webView.loadUrl("https://localhost:3002");
    webView.loadUrl("app://local/index.html");
    webViewMessagingHub.start(parametersManager, &webView);
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
createWebViewEditorView(Steinberg::Vst::EditController *controller,
                        ParametersManager *parametersManager) {
  return new WebViewEditorView(controller, parametersManager);
}