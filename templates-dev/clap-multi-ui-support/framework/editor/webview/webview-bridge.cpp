#include "webview-bridge.h"
#include "../interfaces.h"
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
  std::string paramKey;
};
struct RxMsgPerformEdit {
  std::string type = "performEdit";
  std::string paramKey;
  double value;
};
struct RxMsgEndEdit {
  std::string type = "endEdit";
  std::string paramKey;
};
struct RxMsgInstantEdit {
  std::string type = "instantEdit";
  std::string paramKey;
  double value;
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
  std::string paramKey;
  double value;
};
struct TxMsgBulkSendParameters {
  std::string type = "bulkSendParameters";
  std::map<std::string, double> parameters;
};
struct TxMsgHostNoteOn {
  std::string type = "hostNoteOn";
  int noteNumber;
  double velocity;
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
  int hostNoteSubscriptionToken = -1;

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
      std::map<std::string, double> parameters;
      controllerFacade.getAllParameters(parameters);
      TxMsgBulkSendParameters msg{
          .type = "bulkSendParameters",
          .parameters = parameters,
      };
      sendMessageToWebView(msg);
    } else if (auto *m = std::get_if<RxMsgBeginEdit>(&rxMessage)) {
      controllerFacade.applyParameterEditFromUi(m->paramKey, 0.f,
                                                ParameterEditState::Begin);
    } else if (auto *m = std::get_if<RxMsgPerformEdit>(&rxMessage)) {
      controllerFacade.applyParameterEditFromUi(m->paramKey, m->value,
                                                ParameterEditState::Perform);
    } else if (auto *m = std::get_if<RxMsgEndEdit>(&rxMessage)) {
      controllerFacade.applyParameterEditFromUi(m->paramKey, 0.f,
                                                ParameterEditState::End);
    } else if (auto *m = std::get_if<RxMsgInstantEdit>(&rxMessage)) {
      controllerFacade.applyParameterEditFromUi(
          m->paramKey, m->value, ParameterEditState::InstantChange);
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

  void handleParameterChangeFromController(const std::string &paramKey,
                                           float value) {
    TxMsgSetParameter msg{
        .type = "setParameter",
        .paramKey = paramKey,
        .value = value,
    };
    sendMessageToWebView(msg);
  }

  void handleHostNote(int noteNumber, double velocity) {
    if (velocity > 0.0) {
      TxMsgHostNoteOn msg{
          .type = "hostNoteOn",
          .noteNumber = noteNumber,
          .velocity = velocity,
      };
      sendMessageToWebView(msg);
    } else {
      TxMsgHostNoteOff msg{
          .type = "hostNoteOff",
          .noteNumber = noteNumber,
      };
      sendMessageToWebView(msg);
    }
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
            [this](std::string paramKey, double value) {
              handleParameterChangeFromController(paramKey, value);
            });

    hostNoteSubscriptionToken = controllerFacade.subscribeHostNote(
        [this](int noteNumber, double velocity) {
          handleHostNote(noteNumber, velocity);
        });
    controllerFacade.incrementViewCount();
  }

  void teardown() override {
    controllerFacade.decrementViewCount();
    if (parameterChangeSubscriptionToken != -1) {
      controllerFacade.unsubscribeParameterChange(
          parameterChangeSubscriptionToken);
      parameterChangeSubscriptionToken = -1;
    }
    if (hostNoteSubscriptionToken != -1) {
      controllerFacade.unsubscribeHostNote(hostNoteSubscriptionToken);
      hostNoteSubscriptionToken = -1;
    }
    webViewIo.setMessageReceiver(nullptr);
  }
};

WebViewBridge *WebViewBridge::create(IControllerFacade &controllerFacade,
                                     IWebViewIo &webViewIo) {
  return new WebViewBridgeImpl(controllerFacade, webViewIo);
}

} // namespace sonic