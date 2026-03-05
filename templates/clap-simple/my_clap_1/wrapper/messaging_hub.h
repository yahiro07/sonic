#pragma once

#include "sonic_common/general/mac_web_view.h"
#include <functional>
#include <glaze/glaze.hpp>
#include <string>

class IDownStreamEventPort {};

class IParametersManager {};

class MessagingHub {
private:
  sonic_common::IWebViewIo &webViewIo;
  IDownStreamEventPort &downStreamEventPort;
  IParametersManager &parametersManager;

public:
  MessagingHub(sonic_common::IWebViewIo &webViewIo,
               IDownStreamEventPort &downStreamEventPort,
               IParametersManager &parametersManager)
      : webViewIo(webViewIo), downStreamEventPort(downStreamEventPort),
        parametersManager(parametersManager) {}
  void start() {
    webViewIo.setMessageReceiver(
        [this](const std::string &message) { onMessageFromWebView(message); });
  }
  void onMessageFromWebView(const std::string &message) {
    printf("message from webview: %s\n", message.c_str());
  }

  void stop() { webViewIo.setMessageReceiver(nullptr); }
};

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
  std::string type = "applyEdit";
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
      "log",     "uiLoaded",    "beginEdit",     "applyEdit",
      "endEdit", "instantEdit", "noteOnRequest", "noteOffRequest",
  };
};
} // namespace glz

inline void messagingHub_dev_handleMessageFromUi(
    const std::string &jsonStr,
    std::function<void(std::string &, double)> setParameterFromUi) {
  printf("message: %s\n", jsonStr.c_str());

  RxMessageVariant rxMessage;
  auto ec = glz::read_json<RxMessageVariant>(rxMessage, jsonStr);
  if (ec)
    return;
  if (auto *m = std::get_if<RxMsgPerformEdit>(&rxMessage)) {
    setParameterFromUi(m->identifier, m->value);
  }
}