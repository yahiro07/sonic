#pragma once
#include "../../../domain/interfaces.h"
#include "./events.h"
#include <functional>

namespace sonic {

class IControllerParameterPortal {

public:
  virtual ~IControllerParameterPortal() = default;
  virtual void
  subscribeParameterChange(std::function<void(int, double)> fn) = 0;
  virtual void unsubscribeParameterChange() = 0;

  virtual void applyParameterEdit(std::string paramKey, double value,
                                  ParameterEditState editState) = 0;
};

class IUpstreamEventBus {
public:
  virtual ~IUpstreamEventBus() = default;
  virtual void pushUpstreamEvent(UpstreamEvent event) = 0;
  virtual bool popUpstreamEvent(UpstreamEvent &event) = 0;
};

class IDownstreamEventBus {
public:
  virtual ~IDownstreamEventBus() = default;
  virtual void pushDownstreamEvent(DownstreamEvent event) = 0;
  virtual bool popDownstreamEvent(DownstreamEvent &event) = 0;
};

} // namespace sonic