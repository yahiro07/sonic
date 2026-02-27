

#include "pluginterfaces/vst/ivstmessage.h"
#include "public.sdk/source/vst/vstcomponentbase.h"
#include <optional>

namespace vst3wf {

using namespace Steinberg;

enum class WrappedMessageType {
  // from processor to controller
  hostNoteOn,
  hostNoteOff,
  // from controller to processor
  noteOnRequestFromEditor,
  noteOffRequestFromEditor,
  pullProcessorSideEvents,

};

typedef struct {
  WrappedMessageType type;
  union {
    struct {
      int noteNumber;
      double velocity;
    } noteOnRequestFromEditor;
    struct {
      int noteNumber;
    } noteOffRequestFromEditor;
    struct {
      int noteNumber;
      double velocity;
    } hostNoteOn;
    struct {
      int noteNumber;
    } hostNoteOff;
  };
} WrappedMessageFromController;

typedef struct {
  WrappedMessageType type;
  union {
    struct {
      int noteNumber;
      double velocity;
    } hostNoteOn;
    struct {
      int noteNumber;
    } hostNoteOff;
  };
} WrappedMessageFromProcessor;

class ProcessorSideMessagingBridge {
private:
  Steinberg::Vst::ComponentBase &audioEffect;

public:
  ProcessorSideMessagingBridge(Steinberg::Vst::ComponentBase &audioEffect)
      : audioEffect(audioEffect) {}

  std::optional<WrappedMessageFromController>
  decodeMessage(Steinberg::Vst::IMessage *message) {
    if (strcmp(message->getMessageID(), "noteOnRequestFromEditor") == 0) {
      int64 noteNumber;
      double velocity;
      if (message->getAttributes()->getInt("noteNumber", noteNumber) !=
          kResultOk) {
        return std::nullopt;
      }
      if (message->getAttributes()->getFloat("velocity", velocity) !=
          kResultOk) {
        return std::nullopt;
      }
      return WrappedMessageFromController{
          .type = WrappedMessageType::noteOnRequestFromEditor,
          .noteOnRequestFromEditor = {.noteNumber =
                                          static_cast<int>(noteNumber),
                                      .velocity = velocity},
      };
    } else if (strcmp(message->getMessageID(), "noteOffRequestFromEditor") ==
               0) {
      int64 noteNumber;
      if (message->getAttributes()->getInt("noteNumber", noteNumber) !=
          kResultOk) {
        return std::nullopt;
      }
      return WrappedMessageFromController{
          .type = WrappedMessageType::noteOffRequestFromEditor,
          .noteOffRequestFromEditor = {.noteNumber =
                                           static_cast<int>(noteNumber)},
      };
    } else if (strcmp(message->getMessageID(), "pullProcessorSideEvents") ==
               0) {
      return WrappedMessageFromController{
          .type = WrappedMessageType::pullProcessorSideEvents,
      };
    }
    return std::nullopt;
  }

  void sendHostNoteOn(int noteNumber, double velocity) {
    if (auto msg = owned(audioEffect.allocateMessage())) {
      msg->setMessageID("hostNoteOn");
      msg->getAttributes()->setInt("noteNumber", noteNumber);
      msg->getAttributes()->setFloat("velocity", velocity);
      audioEffect.sendMessage(msg);
    }
  }

  void sendHostNoteOff(int noteNumber) {
    if (auto msg = owned(audioEffect.allocateMessage())) {
      msg->setMessageID("hostNoteOff");
      msg->getAttributes()->setInt("noteNumber", noteNumber);
      audioEffect.sendMessage(msg);
    }
  }
};

class ControllerSideMessagingBridge {
private:
  Steinberg::Vst::ComponentBase &editController;

public:
  ControllerSideMessagingBridge(Steinberg::Vst::ComponentBase &editController)
      : editController(editController) {}

  std::optional<WrappedMessageFromProcessor>
  decodeMessage(Steinberg::Vst::IMessage *message) {
    if (strcmp(message->getMessageID(), "hostNoteOn") == 0) {
      int64 noteNumber;
      double velocity;
      if (message->getAttributes()->getInt("noteNumber", noteNumber) !=
          kResultOk) {
        return std::nullopt;
      }
      if (message->getAttributes()->getFloat("velocity", velocity) !=
          kResultOk) {
        return std::nullopt;
      }
      return WrappedMessageFromProcessor{
          .type = WrappedMessageType::hostNoteOn,
          .hostNoteOn = {.noteNumber = static_cast<int>(noteNumber),
                         .velocity = velocity},
      };
    } else if (strcmp(message->getMessageID(), "hostNoteOff") == 0) {
      int64 noteNumber;
      if (message->getAttributes()->getInt("noteNumber", noteNumber) !=
          kResultOk) {
        return std::nullopt;
      }
      return WrappedMessageFromProcessor{
          .type = WrappedMessageType::hostNoteOff,
          .hostNoteOff = {.noteNumber = static_cast<int>(noteNumber)},
      };
    }
    return std::nullopt;
  }

  void sendNoteOnRequest(int noteNumber, double velocity) {
    if (auto msg = owned(editController.allocateMessage())) {
      msg->setMessageID("noteOnRequestFromEditor");
      msg->getAttributes()->setInt("noteNumber", noteNumber);
      msg->getAttributes()->setFloat("velocity", velocity);
      editController.sendMessage(msg);
    }
  }

  void sendNoteOffRequest(int noteNumber) {
    if (auto msg = owned(editController.allocateMessage())) {
      msg->setMessageID("noteOffRequestFromEditor");
      msg->getAttributes()->setInt("noteNumber", noteNumber);
      editController.sendMessage(msg);
    }
  }

  void sendPullProcessorSideEventsRequest() {
    if (auto msg = owned(editController.allocateMessage())) {
      msg->setMessageID("pullProcessorSideEvents");
      editController.sendMessage(msg);
    }
  }
};

} // namespace vst3wf