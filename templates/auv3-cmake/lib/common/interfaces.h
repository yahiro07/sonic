#pragma once
#include "./events.h"
#include "./parameter_item.h"
#include "advanced_synthesizer.h"
#include "synthesizer_base.h"
#include <functional>
#include <map>

namespace sonic_common {

// using IPluginSynthesizer = SynthesizerBase;
using IPluginSynthesizer = AdvancedSynthesizer;

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

class IParameterManager {
public:
  virtual int subscribeParameterChange(
      std::function<void(const std::string identifier, double value)>
          callback) = 0;
  virtual void unsubscribeParameterChange(int subscriptionId) = 0;

  virtual void getAllParameters(std::map<std::string, double> &parameters) = 0;
};

class IDownstreamEventPort {
public:
  virtual int
  subscribeDownstreamEvent(std::function<void(DownstreamEvent &)> callback) = 0;
  virtual void unsubscribeDownstreamEvent(int subscriptionId) = 0;
};

enum class ParameterEditState {
  Begin,
  Perform,
  End,
  InstantChange,
};

class IUpStreamEventPort {
public:
  virtual void applyParameterEditFromUi(std::string identifier, double value,
                                        ParameterEditState editState) = 0;
  virtual void requestNoteOn(int noteNumber, double velocity) = 0;
  virtual void requestNoteOff(int noteNumber) = 0;
};

class IEventBridge {
public:
  virtual void pushUpstreamEvent(UpstreamEvent &e) = 0;
  virtual bool popUpstreamEvent(UpstreamEvent &e) = 0;

  virtual void pushDownstreamEvent(DownstreamEvent &e) = 0;
  virtual bool popDownstreamEvent(DownstreamEvent &e) = 0;
};

class ITelemetrySupport {
public:
  virtual int subscribeTelemetryData(
      std::function<void(int id, std::vector<float> &buffer)> callback) = 0;
  virtual void unsubscribeTelemetryData(int subscriptionId) = 0;

  virtual void setupPollingTelemetries(int targetBitFlags,
                                       int timerIntervalMs) = 0;
  virtual void stopPollingTelemetries() = 0;
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
