#pragma once
#include <stdint.h>

namespace sonic {

enum class DownstreamEventType {
  HostNote,
  ParameterChange,
  // HostTempo,
  // HostPlayState
};

struct DownstreamEvent {
  DownstreamEventType type;
  union {
    struct {
      int noteNumber;
      double velocity; // 0 for note off
    } note;
    struct {
      uint32_t paramId;
      double value;
    } param;
  };
};

enum class UpstreamEventType {
  ParameterBeginEdit,
  ParameterApplyEdit,
  ParameterEndEdit,
  ParameterInstantChange,
  NoteRequest,
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
      double velocity; // 0 for note off
    } note;
  };
};

} // namespace sonic
