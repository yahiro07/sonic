#pragma once
#include "./webview-io.h"
#include <sonic/core/editor-interfaces.h>

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