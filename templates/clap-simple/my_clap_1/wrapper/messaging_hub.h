#pragma once

#include "sonic_common/logic/parameter_definitions_provider.h"
#include <functional>
#include <glaze/glaze.hpp>
#include <string>

enum class UpStreamEventType {
  parameterBeginEdit,
  parameterApplyEdit,
  parameterEndEdit,
  noteOnRequest,
  noteOffRequest,
};

struct UpstreamEvent {
  UpStreamEventType type;
  union {
    struct {
      uint32_t paramId;
      double value;
    } param;
    struct {
      int noteNumber;
      double velocity;
    } note;
  };
};

enum class DownStreamEventType {
  parameterChange,
  hostNoteOn,
  hostNoteOff,
};

struct DownstreamEvent {
  DownStreamEventType type;
  union {
    struct {
      uint32_t paramId;
      double value;
    } param;
    struct {
      int noteNumber;
      double velocity;
    } note;
  };
};

// class IDownStreamEventPort {};

// class IParametersManager {};

// class MessagingHub {
// private:
//   sonic_common::IWebViewIo &webViewIo;
//   IDownStreamEventPort &downStreamEventPort;
//   IParametersManager &parametersManager;

// public:
//   MessagingHub(sonic_common::IWebViewIo &webViewIo,
//                IDownStreamEventPort &downStreamEventPort,
//                IParametersManager &parametersManager)
//       : webViewIo(webViewIo), downStreamEventPort(downStreamEventPort),
//         parametersManager(parametersManager) {}
//   void start() {
//     webViewIo.setMessageReceiver(
//         [this](const std::string &message) { onMessageFromWebView(message);
//         });
//   }
//   void onMessageFromWebView(const std::string &message) {
//     printf("message from webview: %s\n", message.c_str());
//   }

//   void stop() { webViewIo.setMessageReceiver(nullptr); }
// };

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
  double value;
};
struct RxMsgEndEdit {
  std::string type = "endEdit";
  std::string identifier;
};
struct RxMsgInstantEdit {
  std::string type = "instantEdit";
  std::string identifier;
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

using RxMessageVariant =
    std::variant<RxMsgLog, RxMsgUiLoaded, RxMsgBeginEdit, RxMsgPerformEdit,
                 RxMsgEndEdit, RxMsgInstantEdit, RxMsgNoteOnRequest,
                 RxMsgNoteOffRequest>;
namespace glz {
template <> struct meta<RxMessageVariant> {
  static constexpr std::string_view tag = "type";
  static constexpr auto ids = std::array{
      "log",     "uiLoaded",    "beginEdit",     "performEdit",
      "endEdit", "instantEdit", "noteOnRequest", "noteOffRequest",
  };
};
} // namespace glz

struct TxMsgSetParameter {
  std::string type = "setParameter";
  std::string identifier;
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

// template <typename T>
// static void sendWebViewJsonMessage(IWebViewIo *webView, T &msg) {
//   std::string buffer{};
//   auto ec = glz::write_json(msg, buffer);
//   if (ec)
//     return;
//   // logger.log("send message to ui: %s", buffer.c_str());
//   webView->sendMessage(buffer);
// }

inline void messagingHub_dev_handleMessageFromUi(
    const std::string &jsonStr,
    std::function<void(std::string &, double)> performParameterEditFromUi,
    std::function<void(UpstreamEvent &)> emitUpstreamEvent) {
  printf("message: %s\n", jsonStr.c_str());

  RxMessageVariant rxMessage;
  auto ec = glz::read_json<RxMessageVariant>(rxMessage, jsonStr);
  if (ec)
    return;
  if (auto *m = std::get_if<RxMsgPerformEdit>(&rxMessage)) {
    performParameterEditFromUi(m->identifier, m->value);
  }
}

inline void messagingHub_dev_handleEventFromHost(
    DownstreamEvent &e, std::function<void(std::string &)> sendMessageToWebView,
    sonic_common::ParameterDefinitionsProvider &parameterDefinitionsProvider) {
  if (e.type == DownStreamEventType::parameterChange) {
    printf("downstream param %d %f\n", e.param.paramId, e.param.value);
    auto identifier =
        parameterDefinitionsProvider.getIdentifierByAddress(e.param.paramId);
    if (identifier == "")
      return;
    TxMsgSetParameter msg{
        .type = "setParameter",
        .identifier = identifier,
        .value = e.param.value,
    };
    std::string buffer;
    auto ec = glz::write_json(msg, buffer);
    if (ec)
      return;
    sendMessageToWebView(buffer);
  } else if (e.type == DownStreamEventType::hostNoteOn) {
    printf("downstream noteOn %d %f\n", e.note.noteNumber, e.note.velocity);
  } else if (e.type == DownStreamEventType::hostNoteOff) {
    printf("downstream noteOff %d\n", e.note.noteNumber);
  }
}