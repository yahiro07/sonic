#include "../parameters/edit_controller_parameter_change_notifier.h"
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/gui/iplugview.h"
#include <public.sdk/source/vst/vsteditcontroller.h>

#include "mac_web_view.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include <glaze/glaze.hpp>

using namespace Steinberg;

#include "../project1_cids.h"
#include "../utils/logger.h"

struct MsgSetParameter {
  std::string type = "setParameter";
  int id;
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
  EditControllerParameterChangeNotifier paramChangeNotifier;
  IWebViewIo *webView;
  Vst::EditController *controller;

public:
  void start(Vst::EditController *controller, IWebViewIo *webViewIo) {
    this->controller = controller;
    this->webView = webViewIo;
    webView->setMessageReceiver([this](const std::string &jsonStr) {
      logger.log("message received: %s", jsonStr.c_str());

      auto msg = mapMessageFromUI_fromString(jsonStr);
      if (!msg)
        return;
      if (auto *p = std::get_if<MsgSetParameter>(&*msg)) {
        auto id = p->id;
        auto value = p->value;
        logger.log("set parameter: %d, %f", id, value);
        Vst::ParameterInfo paramInfo;
        this->controller->getParameterInfoByTag(id, paramInfo);
        auto step = paramInfo.stepCount;
        if (step > 0) {
          int idx = static_cast<int>(std::lround(value));
          idx = std::max(0, std::min(idx, step));
          value = static_cast<float>(idx) / static_cast<float>(step);
        } else {
          value = std::max(0.0f, std::min(value, 1.0f));
        }
        this->controller->beginEdit(id);
        this->controller->performEdit(id, value);
        this->controller->endEdit(id);
      } else if (auto *p = std::get_if<MsgUiLoaded>(&*msg)) {
        logger.log("ui loaded");
      }
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