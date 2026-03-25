#pragma once
#include <functional>
#include <memory>

namespace sonic {

enum class ParameterAutomationEventType {
  Value = 0,  // AUParameterAutomationEventTypeValue
  Touch = 1,  // AUParameterAutomationEventTypeTouch
  Release = 2 // AUParameterAutomationEventTypeRelease
};

class ParameterTreeWrapper {
public:
  virtual ~ParameterTreeWrapper() = default;
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

  static std::unique_ptr<ParameterTreeWrapper> create(void *parameterTree);
};

} // namespace sonic
