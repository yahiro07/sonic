#pragma once
#include "../domain/interfaces.h"
#include <memory>

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