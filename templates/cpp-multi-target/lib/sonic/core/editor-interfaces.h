#pragma once
#include "./parameter-spec-item.h"
#include "./types.h"
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

  virtual const ParameterSpecArray &getParameterSpecs() = 0;
};

class IEditorInstance {
public:
  virtual ~IEditorInstance() = default;
  virtual void setup(std::string loadTargetSpec) = 0;
  virtual void teardown() = 0;
  virtual void attachToParent(void *parent) = 0;
  virtual void removeFromParent() = 0;
  virtual void setFrame(int x, int y, int width, int height) = 0;
};

typedef std::function<std::unique_ptr<IEditorInstance>(
    IControllerFacade &controllerFacade)>
    EditorFactoryFn;

} // namespace sonic