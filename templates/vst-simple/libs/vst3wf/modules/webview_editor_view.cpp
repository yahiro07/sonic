#include "../general/logger.h"
#include "../general/mac_web_view.h"
#include "./event_hub.h"
#include "./parameters_manager.h"
#include <glaze/glaze.hpp>
#include <pluginterfaces/base/funknown.h>
#include <pluginterfaces/gui/iplugview.h>
#include <pluginterfaces/vst/ivsteditcontroller.h>
#include <public.sdk/source/vst/vsteditcontroller.h>

namespace vst3wf {

using namespace Steinberg;

// messages from ui

struct MsgUiLoaded {
  std::string type = "uiLoaded";
};
struct MsgBeginEdit {
  std::string type = "beginEdit";
  std::string paramKey;
};
struct MsgPerformEdit {
  std::string type = "performEdit";
  std::string paramKey;
  double value;
};
struct MsgEndEdit {
  std::string type = "endEdit";
  std::string paramKey;
};
struct MsgInstantEdit {
  std::string type = "instantEdit";
  std::string paramKey;
  double value;
};
struct MsgNoteOnRequest {
  std::string type = "noteOnRequest";
  int noteNumber;
};
struct MsgNoteOffRequest {
  std::string type = "noteOffRequest";
  int noteNumber;
};

using MessageFromUI =
    std::variant<MsgUiLoaded, MsgBeginEdit, MsgPerformEdit, MsgEndEdit,
                 MsgInstantEdit, MsgNoteOnRequest, MsgNoteOffRequest>;

// messages from app

struct MsgSetParameter {
  std::string type = "setParameter";
  std::string paramKey;
  double value;
};
struct MsgBulkSendParameters {
  std::string type = "bulkSendParameters";
  std::map<std::string, double> parameters;
};
struct MsgHostNoteOn {
  std::string type = "hostNoteOn";
  int noteNumber;
  double velocity;
};
struct MsgHostNoteOff {
  std::string type = "hostNoteOff";
  int noteNumber;
};

static std::optional<MessageFromUI>
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

template <typename T>
static void sendWebViewJsonMessage(IWebViewIo *webView, T &msg) {
  std::string buffer{};
  auto ec = glz::write_json(msg, buffer);
  if (ec)
    return;
  logger.log("send message to ui: %s", buffer.c_str());
  webView->sendMessage(buffer);
}

class WebViewMessagingHub {
private:
  IWebViewIo *webView;
  ParametersManager *parametersManager;
  EventHub *eventHub;
  int parametersManagerSubscriptionId = 0;
  int eventHubSubscriptionId = 0;

public:
  void start(IWebViewIo *webViewIo, ParametersManager *parametersManager,
             EventHub *eventHub) {
    this->parametersManager = parametersManager;
    this->webView = webViewIo;
    this->eventHub = eventHub;
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
        auto parameters = std::map<std::string, double>();
        this->parametersManager->getAllParameterValues(parameters);
        MsgBulkSendParameters msg{.parameters = parameters};
        sendWebViewJsonMessage(webView, msg);
      } else if (auto *p = std::get_if<MsgNoteOnRequest>(&*msg)) {
        logger.log("note on request: %d, %f", p->noteNumber, p->velocity);
        this->eventHub->noteOnFromEditor(p->noteNumber, p->velocity);
      } else if (auto *p = std::get_if<MsgNoteOffRequest>(&*msg)) {
        logger.log("note off request: %d", p->noteNumber);
        this->eventHub->noteOffFromEditor(p->noteNumber);
      }
    });
    parametersManagerSubscriptionId = parametersManager->subscribeFromEditor(
        [this](std::string identifier, double value) {
          MsgSetParameter msg;
          msg.identifier = identifier;
          msg.value = value;
          sendWebViewJsonMessage(webView, msg);
        });
    eventHubSubscriptionId =
        eventHub->subscribeFromEditor([this](DownstreamEvent &event) {
          logger.log("downstream event: %d", event.type);
          if (event.type == DownStreamEventType::hostNoteOn) {
            auto noteNumber = event.noteOn.noteNumber;
            auto velocity = event.noteOn.velocity;
            MsgHostNoteOn msg;
            msg.noteNumber = noteNumber;
            msg.velocity = velocity;
            sendWebViewJsonMessage(webView, msg);
          } else if (event.type == DownStreamEventType::hostNoteOff) {
            auto noteNumber = event.noteOff.noteNumber;
            MsgHostNoteOff msg;
            msg.noteNumber = noteNumber;
            sendWebViewJsonMessage(webView, msg);
          }
        });
  }
  void stop() {
    if (parametersManagerSubscriptionId > 0) {
      parametersManager->unsubscribeFromEditor(parametersManagerSubscriptionId);
    }
    if (eventHubSubscriptionId > 0) {
      eventHub->unsubscribe(eventHubSubscriptionId);
    }
    webView->setMessageReceiver(nullptr);
  }
};

class WebViewEditorView : public Vst::EditorView {
private:
  MacWebView webView;
  ViewRect viewRect{0, 0, 900, 600};
  WebViewMessagingHub webViewMessagingHub;
  EventHub *eventHub;

public:
  IWebViewIo *getWebViewIO() { return &webView; }

  WebViewEditorView(Vst::EditController *controller,
                    ParametersManager *parametersManager, EventHub *eventHub)
      : Vst::EditorView(controller) {
    printf("WebViewEditorView::WebViewEditorView\n");

    // webView.loadUrl("https://localhost:3002");
    webView.loadUrl("app://local/index.html");
    webViewMessagingHub.start(&webView, parametersManager, eventHub);
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
                        ParametersManager *parametersManager,
                        EventHub *eventHub) {
  return new WebViewEditorView(controller, parametersManager, eventHub);
}

} // namespace vst3wf