#pragma once
#include <cstdint>

namespace sonic {

enum class DownstreamEventType {
  HostNote,
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
  };
};

enum class UpstreamEventType {
  NoteRequest,
  ParameterChange,
};

struct UpstreamEvent {
  UpstreamEventType type;
  union {
    struct {
      int noteNumber;
      double velocity; // 0 for note off
    } note;
    struct {
      uint32_t id;
      double value;
    } parameter;
  };
};
} // namespace sonic