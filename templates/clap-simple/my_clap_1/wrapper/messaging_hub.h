#pragma once

#include "sonic_common/general/mac_web_view.h"
#include <functional>
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