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

inline void messagingHub_dev_handleMessageFromUi(
    const std::string &message,
    std::function<void(uint32_t, double)> setParameterFromUi) {
  printf("message: %s\n", message.c_str());
  if (true) {
    uint32_t paramId = 0;
    double paramValue = rand() / (double)RAND_MAX; // debug;
    setParameterFromUi(paramId, paramValue);
  }
}