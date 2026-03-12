#pragma once

#include "../../../common/spsc-queue.h"
#include "../logic/interfaces.h"
#include <pluginterfaces/vst/ivstmessage.h>
#include <public.sdk/source/vst/vstcomponentbase.h>

namespace vst_support {

using namespace sonic;
using namespace Steinberg;
using namespace Steinberg::Vst;

class ControllerSideMessagePort : public IControllerSideMessagePort {
private:
  ComponentBase &editController;

  // not needed SPSC but using this for easiness
  SPSCQueue<DownstreamEvent, 256> receivedDownstreamEvents;

public:
  ControllerSideMessagePort(ComponentBase &editController)
      : editController(editController) {}

  void pushUpstreamEvent(UpstreamEvent e) override {
    if (e.type == UpstreamEventType::NoteRequest) {
      if (auto msg = owned(editController.allocateMessage())) {
        msg->setMessageID("noteRequest");
        msg->getAttributes()->setInt("noteNumber", e.note.noteNumber);
        msg->getAttributes()->setFloat("velocity", e.note.velocity);
        editController.sendMessage(msg);
      }
    }
  }

  bool applyMessageReceived(Steinberg::Vst::IMessage *message) {
    if (strcmp(message->getMessageID(), "hostNote") == 0) {
      Steinberg::int64 noteNumber;
      double velocity;
      if (message->getAttributes()->getInt("noteNumber", noteNumber) !=
          Steinberg::kResultOk) {
        return false;
      }
      if (message->getAttributes()->getFloat("velocity", velocity) !=
          Steinberg::kResultOk) {
        return false;
      }
      receivedDownstreamEvents.push(DownstreamEvent{
          .type = DownstreamEventType::HostNote,
          .note = {.noteNumber = static_cast<int>(noteNumber),
                   .velocity = velocity},
      });
      return true;
    }
    return false;
  };

  bool popDownstreamEvent(DownstreamEvent &e) override {
    return receivedDownstreamEvents.pop(e);
  }
};
} // namespace vst_support