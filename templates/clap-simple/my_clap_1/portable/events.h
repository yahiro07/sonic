#pragma once
#include <stdint.h>

enum class DownstreamEventType {
  HostNoteOn,
  HostNoteOff,
  ParameterChange,
  // HostTempo,
  // HostPlayState
};

struct DownstreamEvent {
  DownstreamEventType type;
  union {
    struct {
      int noteNumber;
      double velocity;
    } note;
    struct {
      uint32_t paramId;
      double value;
    } param;
  };
};

enum class UpstreamEventType {
  parameterBeginEdit,
  parameterApplyEdit,
  parameterEndEdit,
  noteOnRequest,
  noteOffRequest,
};

struct UpstreamEvent {
  UpstreamEventType type;
  union {
    struct {
      uint32_t paramId;
      double value;
    } param;
    struct {
      int noteNumber;
      double velocity;
    } note;
  };
};
