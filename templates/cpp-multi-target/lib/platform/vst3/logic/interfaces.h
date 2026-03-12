#pragma once
#include "../../../domain/interfaces.h"
#include "./events.h"
#include <functional>

namespace sonic {

class IControllerParameterPortal {

public:
  virtual ~IControllerParameterPortal() = default;
  virtual void
  subscribeParameterChange(std::function<void(uint32_t, double)> fn) = 0;
  virtual void unsubscribeParameterChange() = 0;

  virtual void applyParameterEdit(uint32_t paramId, double value,
                                  ParameterEditState editState) = 0;
};

// class IUpstreamEventBus {
// public:
//   virtual ~IUpstreamEventBus() = default;
//   virtual void pushUpstreamEvent(UpstreamEvent event) = 0;
//   virtual bool popUpstreamEvent(UpstreamEvent &event) = 0;
// };

// class IDownstreamEventBus {
// public:
//   virtual ~IDownstreamEventBus() = default;
//   virtual void pushDownstreamEvent(DownstreamEvent event) = 0;
//   virtual bool popDownstreamEvent(DownstreamEvent &event) = 0;
// };

class IProcessorSideMessagePort {
public:
  virtual ~IProcessorSideMessagePort() = default;
  virtual bool popUpstreamEvent(UpstreamEvent &event) = 0;
  virtual void pushDownstreamEvent(DownstreamEvent event) = 0;
};

class IControllerSideMessagePort {
public:
  virtual ~IControllerSideMessagePort() = default;
  virtual bool popDownstreamEvent(DownstreamEvent &event) = 0;
  virtual void pushUpstreamEvent(UpstreamEvent event) = 0;
};

} // namespace sonic