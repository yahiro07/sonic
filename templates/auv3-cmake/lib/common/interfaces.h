#pragma once
#include "./parameter_item.h"
#include <functional>

namespace sonic_common {

class IWebViewIo {
public:
  virtual ~IWebViewIo() = default;
  virtual void sendMessage(const std::string &message) = 0;
  virtual void
  setMessageReceiver(std::function<void(const std::string &)> receiver) = 0;
};

class IPlatformParameterIo {
public:
  virtual ~IPlatformParameterIo() = default;
  virtual void registerParameters(std::vector<ParameterItem> &params) = 0;
  virtual double getParameter(uint64_t address) = 0;
  virtual void setParameter(uint64_t address, double value) = 0;

  virtual void
  setParameterChangeCallback(std::function<void(uint64_t, double)> fn) = 0;
};

} // namespace sonic_common
