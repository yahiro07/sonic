#pragma once

#include "interfaces.h"
#include "sonic_common/general/spsc_queue.h"

class Eventbridge : public IEventBridge {
private:
  sonic_common::SPSCQueue<UpstreamEvent, 32> upstreamEventQueue;
  sonic_common::SPSCQueue<DownstreamEvent, 32> downstreamEventQueue;

  std::function<void()> upstreamEventPushCallback;
  std::function<void()> downstreamEventPushCallback;

public:
  void setUpstreamEventPushCallback(std::function<void()> callback) {
    this->upstreamEventPushCallback = callback;
  }
  void clearUpstreamEventPushCallback() {
    this->upstreamEventPushCallback = nullptr;
  }

  void setDownstreamEventPushCallback(std::function<void()> callback) {
    this->downstreamEventPushCallback = callback;
  }
  void clearDownstreamEventPushCallback() {
    this->downstreamEventPushCallback = nullptr;
  }

  void pushUpstreamEvent(UpstreamEvent &e) override {
    upstreamEventQueue.push(e);
    if (upstreamEventPushCallback) {
      upstreamEventPushCallback();
    }
  }
  bool popUpstreamEvent(UpstreamEvent &e) override {
    return upstreamEventQueue.pop(e);
  }

  void pushDownstreamEvent(DownstreamEvent &e) override {
    downstreamEventQueue.push(e);
    if (downstreamEventPushCallback) {
      downstreamEventPushCallback();
    }
  }
  bool popDownstreamEvent(DownstreamEvent &e) override {
    return downstreamEventQueue.pop(e);
  }
};