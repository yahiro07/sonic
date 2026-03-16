#pragma once
#include "./events.h"

namespace sonic {

class IHostCallbackRequester {
public:
  virtual ~IHostCallbackRequester() = default;
  virtual void requestMainThreadCallback() = 0;
  virtual void requestFlush() = 0;
};

class IEventBridge {
public:
  virtual void pushUpstreamEvent(UpstreamEvent e) = 0;
  virtual bool popUpstreamEvent(UpstreamEvent &e) = 0;

  virtual void pushDownstreamEvent(DownstreamEvent e) = 0;
  virtual bool popDownstreamEvent(DownstreamEvent &e) = 0;
};

} // namespace sonic