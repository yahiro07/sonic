#pragma once
#include <functional>
#include <memory>

namespace sonic_vst {

class IWebViewIo {
public:
  virtual ~IWebViewIo() = default;
  virtual void sendMessage(const std::string &message) = 0;
  virtual void
  setMessageReceiver(std::function<void(const std::string &)> receiver) = 0;
};

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

} // namespace sonic_vst