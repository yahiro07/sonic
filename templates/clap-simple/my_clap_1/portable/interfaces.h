
#pragma once

#include "events.h"
#include <functional>
#include <map>

class IParameterManager {
public:
  virtual int subscribeParameterChange(
      std::function<void(const std::string identifier, double value)>
          callback) = 0;
  virtual void unsubscribeParameterChange(int subscriptionId) = 0;

  virtual void getAllParameters(std::map<std::string, double> &parameters) = 0;
};

class IDownstreamEventPort {
public:
  virtual int
  subscribeDownstreamEvent(std::function<void(DownstreamEvent &)> callback) = 0;
  virtual void unsubscribeDownstreamEvent(int subscriptionId) = 0;
};

enum class ParameterEditState {
  Begin,
  Perform,
  End,
  InstantChange,
};

class IUpStreamEventPort {
public:
  virtual void applyParameterEditFromUi(std::string identifier, double value,
                                        ParameterEditState editState) = 0;
  virtual void requestNoteOn(int noteNumber, double velocity) = 0;
  virtual void requestNoteOff(int noteNumber) = 0;
};

class IEventBridge {
public:
  virtual void pushUpstreamEvent(UpstreamEvent &e) = 0;
  virtual bool popUpstreamEvent(UpstreamEvent &e) = 0;

  virtual void pushDownstreamEvent(DownstreamEvent &e) = 0;
  virtual bool popDownstreamEvent(DownstreamEvent &e) = 0;
};