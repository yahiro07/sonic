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
  virtual float get(uint32_t id) = 0;
  virtual void set(uint32_t id, float value) = 0;
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
  virtual void getAllParameters(std::map<std::string, float> &parameters) = 0;
  virtual void applyParameterEditFromUi(std::string paramKey, float value,
                                        ParameterEditState editState) = 0;
  virtual void requestNoteOn(int noteNumber, float velocity) = 0;
  virtual void requestNoteOff(int noteNumber) = 0;

  virtual int subscribeParameterChange(
      std::function<void(const std::string paramKey, float value)>
          callback) = 0;
  virtual void unsubscribeParameterChange(int subscriptionId) = 0;

  virtual int subscribeHostNote(
      std::function<void(int noteNumber, float velocity)> callback) = 0;
  virtual void unsubscribeHostNote(int subscriptionId) = 0;
};

} // namespace sonic