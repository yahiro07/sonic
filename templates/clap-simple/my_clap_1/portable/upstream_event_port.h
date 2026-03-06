
#include "./events.h"
#include "./interfaces.h"
#include "./parameter_manager.h"
#include "sonic_common/logic/parameter_definitions_provider.h"

class UpstreamEventPort : public IUpStreamEventPort {
  sonic_common::ParameterDefinitionsProvider &parameterDefinitionsProvider;
  ParameterManager &parameterManager;
  IEventBridge &eventBridge;

  void pushUpstreamEvent(UpstreamEvent e) { eventBridge.pushUpstreamEvent(e); }

public:
  UpstreamEventPort(
      sonic_common::ParameterDefinitionsProvider &parameterDefinitionsProvider,
      ParameterManager &parameterManager, IEventBridge &eventBridge)
      : parameterDefinitionsProvider(parameterDefinitionsProvider),
        parameterManager(parameterManager), eventBridge(eventBridge) {}

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
    pushUpstreamEvent({
        .type = UpstreamEventType::NoteOnRequest,
        .note = {.noteNumber = noteNumber, .velocity = velocity},
    });
  }
  void requestNoteOff(int noteNumber) override {
    pushUpstreamEvent({
        .type = UpstreamEventType::NoteOffRequest,
        .note = {.noteNumber = noteNumber, .velocity = 0.0},
    });
  }
};