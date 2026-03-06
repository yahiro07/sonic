
#include "./events.h"
#include "./interfaces.h"
#include "./parameter_manager.h"
#include "sonic_common/logic/parameter_definitions_provider.h"
#include <functional>
#include <map>

class DownstreamEventPort : public IDownstreamEventPort {
private:
  std::map<int, std::function<void(DownstreamEvent &)>>
      downstreamEventListeners;

public:
  int subscribeDownstreamEvent(
      std::function<void(DownstreamEvent &)> callback) override {
    auto id = downstreamEventListeners.size() + 1;
    downstreamEventListeners[id] = callback;
    return id;
  }
  void unsubscribeDownstreamEvent(int subscriptionId) override {
    downstreamEventListeners.erase(subscriptionId);
  }

  void emitDownstreamEvent(DownstreamEvent &e) {
    for (auto &[id, listener] : downstreamEventListeners) {
      listener(e);
    }
  }
};

class UpstreamEventPort : public IUpStreamEventPort {
  sonic_common::ParameterDefinitionsProvider &parameterDefinitionsProvider;
  ParameterManager &parameterManager;
  std::function<void(UpstreamEvent &)> pushUpstreamEventFn;

  void pushUpstreamEvent(UpstreamEvent e) {
    if (pushUpstreamEventFn) {
      pushUpstreamEventFn(e);
    }
  }

public:
  UpstreamEventPort(
      sonic_common::ParameterDefinitionsProvider &parameterDefinitionsProvider,
      ParameterManager &parameterManager)
      : parameterDefinitionsProvider(parameterDefinitionsProvider),
        parameterManager(parameterManager) {}

  void setDestinationFn(std::function<void(UpstreamEvent &)> fn) {
    this->pushUpstreamEventFn = fn;
  }

  void applyParameterEditFromUi(std::string identifier, double value,
                                ParameterEditState editState) override {
    printf("setParameterFromUi: %s %f\n", identifier.c_str(), value);
    auto _address =
        parameterDefinitionsProvider.getAddressByIdentifier(identifier);
    if (!_address)
      return;
    auto paramId = static_cast<uint32_t>(*_address);
    if (editState == ParameterEditState::Begin) {
      pushUpstreamEvent({
          .type = UpstreamEventType::ParameterBeginEdit,
          .param = {.paramId = paramId, .value = .0},
      });
    } else if (editState == ParameterEditState::Perform) {
      parameterManager.setParameter(paramId, value, false);
      pushUpstreamEvent({
          .type = UpstreamEventType::ParameterApplyEdit,
          .param = {.paramId = paramId, .value = value},
      });
    } else if (editState == ParameterEditState::End) {
      pushUpstreamEvent({
          .type = UpstreamEventType::ParameterEndEdit,
          .param = {.paramId = paramId, .value = .0},
      });
    } else if (editState == ParameterEditState::InstantChange) {
      parameterManager.setParameter(paramId, value, false);
      pushUpstreamEvent({
          .type = UpstreamEventType::ParameterApplyEdit,
          .param = {.paramId = paramId, .value = value},
      });
    }
  }
  void requestNoteOn(int noteNumber, double velocity) override {
    UpstreamEvent e{
        .type = UpstreamEventType::NoteOnRequest,
        .note = {.noteNumber = noteNumber, .velocity = velocity},
    };
    pushUpstreamEvent(e);
  }
  void requestNoteOff(int noteNumber) override {
    UpstreamEvent e{
        .type = UpstreamEventType::NoteOffRequest,
        .note = {.noteNumber = noteNumber, .velocity = 0.0},
    };
    pushUpstreamEvent(e);
  }
};