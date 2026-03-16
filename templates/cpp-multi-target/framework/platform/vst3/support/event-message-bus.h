#pragma once

#include "../../../common/spsc-queue.h"
#include "../logic/interfaces.h"
#include <pluginterfaces/vst/ivstmessage.h>
#include <public.sdk/source/vst/vstcomponentbase.h>

namespace vst_support {

using namespace sonic;
using namespace Steinberg;
using namespace Steinberg::Vst;

class ProcessorSideMessagePort : public IProcessorSideMessagePort {
private:
  ComponentBase &component; // supposing audioEffect

public:
  ProcessorSideMessagePort(ComponentBase &component) : component(component) {}

  void sendDownstreamEvent(DownstreamEvent e) override {
    if (e.type == DownstreamEventType::HostNote) {
      if (auto msg = owned(component.allocateMessage())) {
        msg->setMessageID("hostNote");
        msg->getAttributes()->setInt("noteNumber", e.note.noteNumber);
        msg->getAttributes()->setFloat("velocity", e.note.velocity);
        component.sendMessage(msg);
      }
    }
  }

  bool decodeMessage(Steinberg::Vst::IMessage *message, UpstreamEvent &e) {
    if (strcmp(message->getMessageID(), "noteRequest") == 0) {
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
      e = UpstreamEvent{
          .type = UpstreamEventType::NoteRequest,
          .note = {.noteNumber = static_cast<int>(noteNumber),
                   .velocity = velocity},
      };
      return true;
    } else if (strcmp(message->getMessageID(), "pollingProcessorSideEvent") ==
               0) {
      e = UpstreamEvent{
          .type = UpstreamEventType::PollingProcessorSideEvent,
      };
      return true;
    }
    return false;
  };

  bool popUpstreamEventReceived(UpstreamEvent &e) override {
    // dummy
    // audioProcessor directly uses ProcessorSideMessagePort and can call
    // decodeMessage with the signature depending VST types
    return false;
  }
};

class ControllerSideMessagePort : public IControllerSideMessagePort {
private:
  ComponentBase &component; // supposing editController

  // not needed SPSC but using this for easiness
  SPSCQueue<DownstreamEvent, 256> receivedDownstreamEvents;

public:
  ControllerSideMessagePort(ComponentBase &component) : component(component) {}

  void sendUpstreamEvent(UpstreamEvent e) override {
    if (e.type == UpstreamEventType::NoteRequest) {
      if (auto msg = owned(component.allocateMessage())) {
        msg->setMessageID("noteRequest");
        msg->getAttributes()->setInt("noteNumber", e.note.noteNumber);
        msg->getAttributes()->setFloat("velocity", e.note.velocity);
        component.sendMessage(msg);
      }
    } else if (e.type == UpstreamEventType::PollingProcessorSideEvent) {
      if (auto msg = owned(component.allocateMessage())) {
        msg->setMessageID("pollingProcessorSideEvent");
        component.sendMessage(msg);
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

  bool popDownstreamEventReceived(DownstreamEvent &e) override {
    return receivedDownstreamEvents.pop(e);
  }
};
} // namespace vst_support