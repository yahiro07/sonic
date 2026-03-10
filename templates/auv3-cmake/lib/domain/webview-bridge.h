#include "./interfaces.h"

namespace sonic {

class WebViewBridge {
private:
  IControllerFacade &controllerFacade;
  IWebViewIo &webViewIo;
  int parameterChangeSubscriptionToken = -1;

  void handleMessageFromWebView(const std::string &message) {
    printf("Received message from WebView: %s\n", message.c_str());
  }

  void handleParameterChangeFromController(const std::string &identifier,
                                           double value) {
    printf("Parameter changed: %s = %f\n", identifier.c_str(), value);
  }

public:
  WebViewBridge(IControllerFacade &controllerFacade, IWebViewIo &webViewIo)
      : controllerFacade(controllerFacade), webViewIo(webViewIo) {}

  void setup() {
    webViewIo.setMessageReceiver([this](const std::string &message) {
      handleMessageFromWebView(message);
    });
    parameterChangeSubscriptionToken =
        controllerFacade.subscribeParameterChange(
            [this](std::string paramKey, double value) {
              handleParameterChangeFromController(paramKey, value);
            });
  }

  void teardown() {
    if (parameterChangeSubscriptionToken != -1) {
      controllerFacade.unsubscribeParameterChange(
          parameterChangeSubscriptionToken);
      parameterChangeSubscriptionToken = -1;
    }
    webViewIo.setMessageReceiver(nullptr);
  }
};

} // namespace sonic