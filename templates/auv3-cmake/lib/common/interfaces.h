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

class IPluginDomain {
public:
  virtual void initialize() = 0;
  virtual void prepareProcessing(double sampleRate, uint32_t maxFrameCount) = 0;
  virtual void processAudio(float *bufferL, float *bufferR,
                            uint32_t frames) = 0;
  virtual void setParameter(uint64_t address, double value) = 0;
  virtual void noteOn(int noteNumber, double velocity) = 0;
  virtual void noteOff(int noteNumber) = 0;
  virtual void getDesiredEditorSize(uint32_t &width, uint32_t &height) = 0;
};

} // namespace sonic_common
