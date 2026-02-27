#pragma once

#include <functional>
#include <map>
#include <public.sdk/source/vst/vstcomponentbase.h>
#include <thread>

namespace vst3wf {
using namespace Steinberg;

enum class DownStreamEventType {
  hostNoteOn,
  hostNoteOff,
  hostTempo,
  hostPlayState
};

typedef struct {
  DownStreamEventType type;
  union {
    struct {
      int noteNumber;
      double velocity;
    } noteOn;
    struct {
      int noteNumber;
    } noteOff;
    struct {
      double tempo;
    } tempo;
    struct {
      bool isPlaying;
    } playState;
  };
} DownstreamEvent;

class EventsPollingTimer {
private:
  std::function<void()> pollingFn;
  int timerId = 0;
  std::atomic<bool> running{false};
  std::thread th;

public:
  void setPollingFn(std::function<void()> pollingFn) {
    this->pollingFn = pollingFn;
  }
  void clearPollingFn() { pollingFn = nullptr; }

  void start() {
    running = true;
    th = std::thread([this] {
      while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        pollingFn();
      }
    });
  }
  void stop() {
    running = false;
    if (th.joinable())
      th.join();
  }
};

class EventHub {
private:
  Steinberg::Vst::ComponentBase &editController;
  EventsPollingTimer eventsPollingTimer;
  int listenersIdCounter = 0;
  std::map<int, std::function<void(DownstreamEvent &)>> listeners;

  std::vector<DownstreamEvent> downstreamEventsQueue;

public:
  EventHub(Steinberg::Vst::ComponentBase &editController)
      : editController(editController) {
    eventsPollingTimer.setPollingFn([this]() { this->updatePolling(); });
  }
  ~EventHub() { eventsPollingTimer.clearPollingFn(); }

  void notifyFromEditController(Steinberg::Vst::IMessage *message) {
    if (strcmp(message->getMessageID(), "hostNoteOn") == 0) {
      int64 noteNumber;
      double velocity;
      if (message->getAttributes()->getInt("noteNumber", noteNumber) !=
          kResultOk) {
        return;
      }
      if (message->getAttributes()->getFloat("velocity", velocity) !=
          kResultOk) {
        return;
      }
      DownstreamEvent event;
      event.type = DownStreamEventType::hostNoteOn;
      event.noteOn.noteNumber = noteNumber;
      event.noteOn.velocity = velocity;
      downstreamEventsQueue.push_back(event);
    } else if (strcmp(message->getMessageID(), "hostNoteOff") == 0) {
      int64 noteNumber;
      if (message->getAttributes()->getInt("noteNumber", noteNumber) !=
          kResultOk) {
        return;
      }
      DownstreamEvent event;
      event.type = DownStreamEventType::hostNoteOff;
      event.noteOff.noteNumber = noteNumber;
      downstreamEventsQueue.push_back(event);
    }
  }

  int subscribeFromEditor(std::function<void(DownstreamEvent &)> callback) {
    int id = ++listenersIdCounter;
    listeners[id] = callback;
    if (listeners.size() == 1) {
      eventsPollingTimer.start();
    }
    return id;
  }
  void unsubscribe(int id) {
    listeners.erase(id);
    if (listeners.empty()) {
      eventsPollingTimer.stop();
    }
  }

  void noteOnFromEditor(int noteNumber, double velocity) {
    Steinberg::Vst::IMessage *msg = editController.allocateMessage();
    msg->setMessageID("noteOnRequestFromEditor");
    msg->getAttributes()->setInt("noteNumber", noteNumber);
    msg->getAttributes()->setFloat("velocity", velocity);
    editController.sendMessage(msg);
  }
  void noteOffFromEditor(int noteNumber) {
    Steinberg::Vst::IMessage *msg = editController.allocateMessage();
    msg->setMessageID("noteOffRequestFromEditor");
    msg->getAttributes()->setInt("noteNumber", noteNumber);
    editController.sendMessage(msg);
  }

  void updatePolling() {
    for (auto &event : downstreamEventsQueue) {
      for (auto &listener : listeners) {
        listener.second(event);
      }
    }
    downstreamEventsQueue.clear();

    Steinberg::Vst::IMessage *msg = editController.allocateMessage();
    msg->setMessageID("pullProcessorSideEvents");
    editController.sendMessage(msg);
  }
};

} // namespace vst3wf