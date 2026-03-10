#include "webview-bridge.h"
#include "./interfaces.h"
#include <glaze/glaze.hpp>

namespace sonic {

struct RxMsgLog {
  std::string type = "log";
  double timestamp;
  std::string logKind;
  std::string message;
};
struct RxMsgUiLoaded {
  std::string type = "uiLoaded";
};
struct RxMsgBeginEdit {
  std::string type = "beginEdit";
  std::string identifier;
};
struct RxMsgPerformEdit {
  std::string type = "performEdit";
  std::string identifier;
  float value;
};
struct RxMsgEndEdit {
  std::string type = "endEdit";
  std::string identifier;
};
struct RxMsgInstantEdit {
  std::string type = "instantEdit";
  std::string identifier;
  float value;
};
struct RxMsgNoteOnRequest {
  std::string type = "noteOnRequest";
  int noteNumber;
};
struct RxMsgNoteOffRequest {
  std::string type = "noteOffRequest";
  int noteNumber;
};
struct RxMsgSetupPollingTelemetries {
  std::string type = "setupPollingTelemetries";
  int targetBitFlags;
  int timerIntervalMs;
};
struct RxMsgStopPollingTelemetries {
  std::string type = "stopPollingTelemetries";
};

using RxMessageVariant =
    std::variant<RxMsgLog, RxMsgUiLoaded, RxMsgBeginEdit, RxMsgPerformEdit,
                 RxMsgEndEdit, RxMsgInstantEdit, RxMsgNoteOnRequest,
                 RxMsgNoteOffRequest, RxMsgSetupPollingTelemetries,
                 RxMsgStopPollingTelemetries>;

struct TxMsgSetParameter {
  std::string type = "setParameter";
  std::string identifier;
  float value;
};
struct TxMsgBulkSendParameters {
  std::string type = "bulkSendParameters";
  std::map<std::string, float> parameters;
};
struct TxMsgHostNoteOn {
  std::string type = "hostNoteOn";
  int noteNumber;
  float velocity;
};
struct TxMsgHostNoteOff {
  std::string type = "hostNoteOff";
  int noteNumber;
};
struct TxMsgTelemetryData {
  std::string type = "telemetryData";
  std::vector<float> buffer;
};

} // namespace sonic

namespace glz {
template <> struct meta<sonic::RxMessageVariant> {
  static constexpr std::string_view tag = "type";
  static constexpr auto ids = std::array{
      "log",
      "uiLoaded",
      "beginEdit",
      "performEdit",
      "endEdit",
      "instantEdit",
      "noteOnRequest",
      "noteOffRequest",
      "setupPollingTelemetries",
      "stopPollingTelemetries",
  };
};
} // namespace glz

namespace sonic {

class WebViewBridgeImpl : public WebViewBridge {
private:
  IControllerFacade &controllerFacade;
  IWebViewIo &webViewIo;
  int parameterChangeSubscriptionToken = -1;

  template <typename T> void sendMessageToWebView(T &msg) {
    std::string buffer{};
    auto ec = ::glz::write_json(msg, buffer);
    if (ec)
      return;
    // logger.log("send message to ui: %s", buffer.c_str());
    webViewIo.sendMessage(buffer);
  }

  void handleMessageFromWebView(const std::string &jsonStr) {
    printf("message: %s\n", jsonStr.c_str());

    RxMessageVariant rxMessage;
    auto ec = glz::read_json<RxMessageVariant>(rxMessage, jsonStr);
    if (ec)
      return;

    if (auto *m = std::get_if<RxMsgLog>(&rxMessage)) {
      printf("log from webview: [%s] %s\n", m->logKind.c_str(),
             m->message.c_str());
    } else if (auto *m = std::get_if<RxMsgUiLoaded>(&rxMessage)) {
      printf("ui loaded\n");
      std::map<std::string, float> parameters;
      controllerFacade.getAllParameters(parameters);
      TxMsgBulkSendParameters msg{
          .type = "bulkSendParameters",
          .parameters = parameters,
      };
      sendMessageToWebView(msg);
    } else if (auto *m = std::get_if<RxMsgBeginEdit>(&rxMessage)) {
      controllerFacade.applyParameterEditFromUi(m->identifier, 0.f,
                                                ParameterEditState::Begin);
    } else if (auto *m = std::get_if<RxMsgPerformEdit>(&rxMessage)) {
      controllerFacade.applyParameterEditFromUi(m->identifier, m->value,
                                                ParameterEditState::Perform);
    } else if (auto *m = std::get_if<RxMsgEndEdit>(&rxMessage)) {
      controllerFacade.applyParameterEditFromUi(m->identifier, 0.f,
                                                ParameterEditState::End);
    } else if (auto *m = std::get_if<RxMsgInstantEdit>(&rxMessage)) {
      controllerFacade.applyParameterEditFromUi(
          m->identifier, m->value, ParameterEditState::InstantChange);
    } else if (auto *m = std::get_if<RxMsgNoteOnRequest>(&rxMessage)) {
      controllerFacade.requestNoteOn(m->noteNumber, 1.f);
    } else if (auto *m = std::get_if<RxMsgNoteOffRequest>(&rxMessage)) {
      controllerFacade.requestNoteOff(m->noteNumber);
    } else if (auto *m =
                   std::get_if<RxMsgSetupPollingTelemetries>(&rxMessage)) {
      // controllerFacade.setupPollingTelemetries(m->targetBitFlags,
      //                                          m->timerIntervalMs);
    } else if (auto *m = std::get_if<RxMsgStopPollingTelemetries>(&rxMessage)) {
      // controllerFacade.stopPollingTelemetries();
    }
  }

  void handleParameterChangeFromController(const std::string &identifier,
                                           float value) {
    TxMsgSetParameter msg{
        .type = "setParameter",
        .identifier = identifier,
        .value = value,
    };
    sendMessageToWebView(msg);
  }

public:
  WebViewBridgeImpl(IControllerFacade &controllerFacade, IWebViewIo &webViewIo)
      : controllerFacade(controllerFacade), webViewIo(webViewIo) {}

  void setup() override {
    webViewIo.setMessageReceiver([this](const std::string &message) {
      handleMessageFromWebView(message);
    });
    parameterChangeSubscriptionToken =
        controllerFacade.subscribeParameterChange(
            [this](std::string paramKey, float value) {
              handleParameterChangeFromController(paramKey, value);
            });
  }

  void teardown() override {
    if (parameterChangeSubscriptionToken != -1) {
      controllerFacade.unsubscribeParameterChange(
          parameterChangeSubscriptionToken);
      parameterChangeSubscriptionToken = -1;
    }
    webViewIo.setMessageReceiver(nullptr);
  }
};

WebViewBridge *WebViewBridge::create(IControllerFacade &controllerFacade,
                                     IWebViewIo &webViewIo) {
  return new WebViewBridgeImpl(controllerFacade, webViewIo);
}

} // namespace sonic