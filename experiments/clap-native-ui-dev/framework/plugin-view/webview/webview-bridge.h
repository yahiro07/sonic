#pragma once
#include "../../core/editor-interfaces.h"
#include "./webview-io.h"

namespace sonic {

class WebViewBridge {
public:
  static WebViewBridge *create(IControllerFacade &controllerFacade,
                               IWebViewIo &webViewIo);
  virtual ~WebViewBridge() = default;
  virtual void setup() = 0;
  virtual void teardown() = 0;
};

} // namespace sonic