#pragma once
#include <functional>

namespace sonic {

enum class ParameterAutomationEventType {
  Value = 0,  // AUParameterAutomationEventTypeValue
  Touch = 1,  // AUParameterAutomationEventTypeTouch
  Release = 2 // AUParameterAutomationEventTypeRelease
};

class IParameterTreeWrapper {
public:
  virtual ~IParameterTreeWrapper() = default;
  virtual void setImplementorValueObserver(
      std::function<void(uint64_t address, float value)> observer) = 0;
  virtual void setImplementorValueProvider(
      std::function<float(uint64_t address)> provider) = 0;

  virtual void setParameterValue(uint64_t address, float value,
                                 void *originator,
                                 ParameterAutomationEventType eventType) = 0;

  virtual float getParameterValue(uint64_t address) = 0;

  virtual void *tokenByAddingParameterObserver(
      std::function<void(uint64_t address, float value)> observer) = 0;
  virtual void removeParameterObserver(void *observerToken) = 0;
};

IParameterTreeWrapper *createParameterTreeWrapper(void *parameterTree);
void destroyParameterTreeWrapper(IParameterTreeWrapper *wrapper);

} // namespace sonic