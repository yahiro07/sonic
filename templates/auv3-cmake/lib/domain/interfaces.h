#pragma once

#include "../api/synthesizer-base.h"
#include <functional>

namespace sonic {
using IPluginSynthesizer = SynthesizerBase;

class IWebViewIo {
public:
  virtual ~IWebViewIo() = default;
  virtual void sendMessage(const std::string &message) = 0;
  virtual void
  setMessageReceiver(std::function<void(const std::string &)> receiver) = 0;
};

} // namespace sonic