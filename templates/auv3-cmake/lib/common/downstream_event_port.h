#pragma once

#include "./events.h"
#include "./interfaces.h"
#include <functional>
#include <map>

namespace sonic_common {

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

} // namespace sonic_common