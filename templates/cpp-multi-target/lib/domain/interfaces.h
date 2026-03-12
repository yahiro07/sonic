#pragma once

#include "../api/synthesizer-base.h"
#include <functional>
#include <map>

namespace sonic {

using IPluginSynthesizer = SynthesizerBase;

class IWebViewIo {
public:
  virtual ~IWebViewIo() = default;
  virtual void sendMessage(const std::string &message) = 0;
  virtual void
  setMessageReceiver(std::function<void(const std::string &)> receiver) = 0;
};

class IParametersStore {
public:
  virtual ~IParametersStore() = default;
  virtual double get(uint32_t id) = 0;
  virtual void set(uint32_t id, double value) = 0;
};

// class IParameterManager {
// public:
//   virtual ~IParameterManager() = default;

//   virtual void setParameter(ParamId id, float value, bool notifyToUi) = 0;
//   virtual float getParameter(ParamId id) = 0;

//   virtual void getAllParameters(std::map<std::string, float> &parameters) =
//   0;

//   virtual int subscribeParameterChange(
//       std::function<void(const std::string paramKey, float value)>
//           callback) = 0;
//   virtual void unsubscribeParameterChange(int subscriptionId) = 0;
// };

enum class ParameterEditState {
  Begin,
  Perform,
  End,
  InstantChange,
};

class IControllerFacade {
public:
  virtual ~IControllerFacade() = default;
  virtual void getAllParameters(std::map<std::string, double> &parameters) = 0;
  virtual void applyParameterEditFromUi(std::string paramKey, double value,
                                        ParameterEditState editState) = 0;
  virtual void requestNoteOn(int noteNumber, double velocity) = 0;
  virtual void requestNoteOff(int noteNumber) = 0;

  virtual int subscribeParameterChange(
      std::function<void(const std::string paramKey, double value)>
          callback) = 0;
  virtual void unsubscribeParameterChange(int subscriptionId) = 0;

  virtual int subscribeHostNote(
      std::function<void(int noteNumber, double velocity)> callback) = 0;
  virtual void unsubscribeHostNote(int subscriptionId) = 0;

  virtual void incrementViewCount() = 0;
  virtual void decrementViewCount() = 0;
};

} // namespace sonic