#pragma once
#include <cstdint>

enum class AudioProcessorEventType { None = 0, Parameter };

struct AudioProcessorEvent {
  AudioProcessorEventType type;
  uint64_t address;
  float value;
};
