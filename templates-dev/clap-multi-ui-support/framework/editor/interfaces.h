#pragma once
#include "../core/types.h"
#include <functional>
#include <map>

namespace sonic {

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