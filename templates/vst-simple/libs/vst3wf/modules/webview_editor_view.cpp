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

// Used only for discriminating MessageFromUI by `type`.
struct MsgTypeHeader {
  std::string type;
};

static std::optional<MessageFromUI>
mapMessageFromUI_fromString(const std::string &jsonStr) {
  // NOTE: glz deserializing directly into a std::variant chooses the first
  // alternative that matches the JSON shape. MsgNoteOnRequest and
  // MsgNoteOffRequest have identical fields, so we must discriminate by `type`.
  MsgTypeHeader header;
  // Header decode must allow unknown keys because real messages include more
  // fields than just `type`.
  {
    glz::context ctx{};
    if (auto ec = glz::read<glz::opts{.error_on_unknown_keys = false}>(
            header, jsonStr, ctx);
        ec) {
      return std::nullopt;
    }
  }

  auto decode = [&](auto &out) -> bool {
    if (auto ec = glz::read_json(out, jsonStr); ec) {
      logger.log("messageFromUI_fromJson(%s): %s", header.type.c_str(),
                 glz::format_error(ec, jsonStr).c_str());
      return false;
    }
    return true;
  };

  if (header.type == "uiLoaded") {
    MsgUiLoaded msg;
    if (!decode(msg))
      return std::nullopt;
    return msg;
  }
  if (header.type == "beginEdit") {
    MsgBeginEdit msg;
    if (!decode(msg))
      return std::nullopt;
    return msg;
  }
  if (header.type == "performEdit") {
    MsgPerformEdit msg;
    if (!decode(msg))
      return std::nullopt;
    return msg;
  }
  if (header.type == "endEdit") {
    MsgEndEdit msg;
    if (!decode(msg))
      return std::nullopt;
    return msg;
  }
  if (header.type == "instantEdit") {
    MsgInstantEdit msg;
    if (!decode(msg))
      return std::nullopt;
    return msg;
  }
  if (header.type == "noteOnRequest") {
    MsgNoteOnRequest msg;
    if (!decode(msg))
      return std::nullopt;
    return msg;
  }
  if (header.type == "noteOffRequest") {
    MsgNoteOffRequest msg;
    if (!decode(msg))
      return std::nullopt;
    return msg;
  }

  logger.log("messageFromUI_fromJson: unknown type '%s'", header.type.c_str());
  return std::nullopt;
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
      if (auto *p = std::get_if<MsgUiLoaded>(&*msg)) {
        logger.log("ui loaded");
        auto parameters = std::map<std::string, double>();
        this->parametersManager->getAllParameterValues(parameters);
        MsgBulkSendParameters msg{.parameters = parameters};
        sendWebViewJsonMessage(webView, msg);
      } else if (auto *p = std::get_if<MsgBeginEdit>(&*msg)) {
        logger.log("begin edit: %s", p->paramKey.c_str());
        auto paramKey = p->paramKey;
        this->parametersManager->applyParameterEdit(
            paramKey, 0, ParameterEditingState::Begin);
      } else if (auto *p = std::get_if<MsgPerformEdit>(&*msg)) {
        logger.log("perform edit: %s, %f", p->paramKey.c_str(), p->value);
        auto paramKey = p->paramKey;
        auto value = p->value;
        this->parametersManager->applyParameterEdit(
            paramKey, value, ParameterEditingState::Perform);
      } else if (auto *p = std::get_if<MsgEndEdit>(&*msg)) {
        logger.log("end edit: %s", p->paramKey.c_str());
        auto paramKey = p->paramKey;
        this->parametersManager->applyParameterEdit(paramKey, 0,
                                                    ParameterEditingState::End);
      } else if (auto *p = std::get_if<MsgInstantEdit>(&*msg)) {
        logger.log("instant edit: %s, %f", p->paramKey.c_str(), p->value);
        auto paramKey = p->paramKey;
        auto value = p->value;
        this->parametersManager->applyParameterEdit(
            paramKey, value, ParameterEditingState::InstantChange);
      } else if (auto *p = std::get_if<MsgNoteOnRequest>(&*msg)) {
        logger.log("note on request: %d %s", p->noteNumber, p->type.c_str());
        this->eventHub->noteOnFromEditor(p->noteNumber, 1.0);
      } else if (auto *p = std::get_if<MsgNoteOffRequest>(&*msg)) {
        logger.log("note off request: %d %s", p->noteNumber, p->type.c_str());
        this->eventHub->noteOffFromEditor(p->noteNumber);
      }
    });
    parametersManagerSubscriptionId = parametersManager->subscribeFromEditor(
        [this](std::string identifier, double value) {
          MsgSetParameter msg;
          msg.paramKey = identifier;
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

    webView.loadUrl("http://localhost:3000?debug=1&dlog=1");
    // webView.loadUrl("app://local/index.html");
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