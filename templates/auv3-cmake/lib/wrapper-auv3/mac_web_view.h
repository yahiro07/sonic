#pragma once
#include "../common/interfaces.h"
#include <functional>
#include <memory>

namespace sonic_common {

class MacWebView : public IWebViewIo {
private:
  class Impl;
  std::unique_ptr<Impl> impl;

public:
  MacWebView();
  ~MacWebView();
  void attachToParent(void *parent);
  void removeFromParent();
  void setFrame(int x, int y, int width, int height);
  void loadUrl(const std::string &url);
  void sendMessage(const std::string &message);
  void setMessageReceiver(std::function<void(const std::string &)> receiver);
};

} // namespace sonic_common