#pragma once

#include "./messaging_helper.h"
#include <functional>
#include <map>
#include <public.sdk/source/vst/vstcomponentbase.h>
#include <thread>

namespace vst3wf {

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
  ControllerSideMessagingBridge messagingBridge;
  EventsPollingTimer eventsPollingTimer;
  int listenersIdCounter = 0;
  std::map<int, std::function<void(DownstreamEvent &)>> listeners;

  std::vector<DownstreamEvent> downstreamEventsQueue;

public:
  EventHub(Steinberg::Vst::ComponentBase &editController)
      : messagingBridge(editController) {
    eventsPollingTimer.setPollingFn([this]() { this->updatePolling(); });
  }
  ~EventHub() { eventsPollingTimer.clearPollingFn(); }

  bool notifyFromEditController(Steinberg::Vst::IMessage *message) {
    auto wm = messagingBridge.decodeMessage(message);
    if (!wm.has_value()) {
      return false;
    }
    if (wm->type == WrappedMessageType::hostNoteOn) {
      DownstreamEvent event({
          .type = DownStreamEventType::hostNoteOn,
          .noteOn = {.noteNumber = wm->hostNoteOn.noteNumber,
                     .velocity = wm->hostNoteOn.velocity},
      });
      downstreamEventsQueue.push_back(event);
    } else if (wm->type == WrappedMessageType::hostNoteOff) {
      DownstreamEvent event({
          .type = DownStreamEventType::hostNoteOff,
          .noteOff = {.noteNumber = wm->hostNoteOff.noteNumber},
      });
      downstreamEventsQueue.push_back(event);
    }
    return true;
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
    messagingBridge.sendNoteOnRequest(noteNumber, velocity);
  }
  void noteOffFromEditor(int noteNumber) {
    messagingBridge.sendNoteOffRequest(noteNumber);
  }

  void updatePolling() {
    for (auto &event : downstreamEventsQueue) {
      for (auto &listenerKv : listeners) {
        listenerKv.second(event);
      }
    }
    downstreamEventsQueue.clear();

    messagingBridge.sendPullProcessorSideEventsRequest();
  }
};

} // namespace vst3wf