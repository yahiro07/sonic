#pragma once

#include "./interfaces.h"
#include "events.h"
#include "sonic_common/general/mac_web_view.h"
#include "sonic_common/logic/parameter_definitions_provider.h"
#include <functional>
#include <glaze/glaze.hpp>
#include <string>

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

inline void messagingHub_dev_handleMessageFromUi(
    const std::string &jsonStr,
    std::function<void(std::string &, double)> performParameterEditFromUi,
    std::function<void(UpstreamEvent &)> emitUpstreamEvent) {}

inline void messagingHub_dev_handleEventFromHost(
    DownstreamEvent &e, std::function<void(std::string &)> sendMessageToWebView,
    sonic_common::ParameterDefinitionsProvider &parameterDefinitionsProvider) {}

class WebViewBridge {

private:
  IParameterManager &parameterManager;
  IUpStreamEventPort &upstreamEventPort;
  IDownstreamEventPort &downstreamEventPort;
  sonic_common::IWebViewIo *webViewIo = nullptr;

  int parameterChangeSubscriptionId = -1;
  int downstreamEventSubscriptionId = -1;

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
    } else if (auto *m = std::get_if<RxMsgBeginEdit>(&rxMessage)) {
      upstreamEventPort.applyParameterEditFromUi(m->identifier, 0.0,
                                                 ParameterEditState::Begin);
    } else if (auto *m = std::get_if<RxMsgPerformEdit>(&rxMessage)) {
      upstreamEventPort.applyParameterEditFromUi(m->identifier, m->value,
                                                 ParameterEditState::Perform);
    } else if (auto *m = std::get_if<RxMsgEndEdit>(&rxMessage)) {
      upstreamEventPort.applyParameterEditFromUi(m->identifier, 0.0,
                                                 ParameterEditState::End);
    } else if (auto *m = std::get_if<RxMsgInstantEdit>(&rxMessage)) {
      upstreamEventPort.applyParameterEditFromUi(
          m->identifier, m->value, ParameterEditState::InstantChange);
    } else if (auto *m = std::get_if<RxMsgNoteOnRequest>(&rxMessage)) {
      upstreamEventPort.requestNoteOn(m->noteNumber, 1.0);
    } else if (auto *m = std::get_if<RxMsgNoteOffRequest>(&rxMessage)) {
      upstreamEventPort.requestNoteOff(m->noteNumber);
    }
  }

  template <typename T> void sendMessageToWebView(T &msg) {
    std::string buffer{};
    auto ec = glz::write_json(msg, buffer);
    if (ec)
      return;
    // logger.log("send message to ui: %s", buffer.c_str());
    webViewIo->sendMessage(buffer);
  }

  void handleParameterChangeFromController(const std::string &identifier,
                                           double value) {
    TxMsgSetParameter msg{
        .type = "setParameter",
        .identifier = identifier,
        .value = value,
    };
    sendMessageToWebView(msg);
  }

  void handleDownstreamEventFromHost(DownstreamEvent &e) {
    if (e.type == DownstreamEventType::ParameterChange) {
      // ignore this since parameter changes are sent to webview in
      // handleParameterChangeFromController
    } else if (e.type == DownstreamEventType::HostNoteOn) {
      TxMsgHostNoteOn msg{
          .type = "hostNoteOn",
          .noteNumber = e.note.noteNumber,
          .velocity = e.note.velocity,
      };
      sendMessageToWebView(msg);
    } else if (e.type == DownstreamEventType::HostNoteOff) {
      TxMsgHostNoteOff msg{
          .type = "hostNoteOff",
          .noteNumber = e.note.noteNumber,
      };
      sendMessageToWebView(msg);
    }
  }

public:
  WebViewBridge(IParameterManager &parameterManager,
                IUpStreamEventPort &upstreamEventPort,
                IDownstreamEventPort &downstreamEventPort)
      : parameterManager(parameterManager),
        upstreamEventPort(upstreamEventPort),
        downstreamEventPort(downstreamEventPort) {}

  void onWebViewOpen(sonic_common::IWebViewIo &webViewIo) {
    this->webViewIo = &webViewIo;

    webViewIo.setMessageReceiver([this](const std::string &message) {
      handleMessageFromWebView(message);
    });
    parameterChangeSubscriptionId = parameterManager.subscribeParameterChange(
        [this](const std::string &identifier, double value) {
          handleParameterChangeFromController(identifier, value);
        });

    downstreamEventSubscriptionId =
        downstreamEventPort.subscribeDownstreamEvent(
            [this](DownstreamEvent &e) { handleDownstreamEventFromHost(e); });
  }
  void onWebViewClose() {
    if (parameterChangeSubscriptionId != -1) {
      parameterManager.unsubscribeParameterChange(
          parameterChangeSubscriptionId);
      parameterChangeSubscriptionId = -1;
    }
    if (downstreamEventSubscriptionId != -1) {
      downstreamEventPort.unsubscribeDownstreamEvent(
          downstreamEventSubscriptionId);
      downstreamEventSubscriptionId = -1;
    }
    if (webViewIo) {
      webViewIo->setMessageReceiver(nullptr);
      webViewIo = nullptr;
    }
  }
};
