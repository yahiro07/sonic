#pragma once

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
      float velocity; // 0 for note off
    } note;
  };
};

enum class UpstreamEventType {
  NoteRequest,
};

struct UpstreamEvent {
  UpstreamEventType type;
  union {
    struct {
      int noteNumber;
      float velocity; // 0 for note off
    } note;
  };
};
} // namespace sonic