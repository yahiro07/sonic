#pragma once
#include "../core/types.h"
#include <functional>
#include <map>
#include <optional>

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
  virtual void getAllParameters(std::map<ParamId, double> &parameters) = 0;
  virtual void applyParameterEditFromUi(ParamId paramId, double value,
                                        ParameterEditState editState) = 0;
  virtual void requestNoteOn(int noteNumber, double velocity) = 0;
  virtual void requestNoteOff(int noteNumber) = 0;

  virtual int subscribeParameterChange(
      std::function<void(ParamId paramId, double value)> callback) = 0;
  virtual void unsubscribeParameterChange(int subscriptionId) = 0;

  virtual int subscribeHostNote(
      std::function<void(int noteNumber, double velocity)> callback) = 0;
  virtual void unsubscribeHostNote(int subscriptionId) = 0;

  virtual void incrementViewCount() = 0;
  virtual void decrementViewCount() = 0;

  virtual std::optional<std::string> getParameterKeyById(ParamId id) = 0;
  virtual std::optional<ParamId>
  getParameterIdByParamKey(std::string paramKey) = 0;
};

} // namespace sonic