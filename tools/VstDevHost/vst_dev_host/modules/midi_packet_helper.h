#pragma once
#include <cstdint>
#include <functional>

namespace vst_dev_host {

const int VARIABLE_LENGTH_SYSEX = -2;

inline int getMidiMessageLength(uint8_t status) {
  const auto upper4 = status & 0xF0;
  if (0x80 <= upper4 && upper4 <= 0xE0) {
    if (upper4 == 0xC0 || upper4 == 0xD0) {
      return 2;
    } else {
      return 3;
    }
  }
  // System Common / Realtime
  switch (status) {
  case 0xF1:
    return 2;
  case 0xF2:
    return 3;
  case 0xF3:
    return 2;
  // 1-byte realtime
  case 0xF6:
  case 0xF8:
  case 0xF9:
  case 0xFA:
  case 0xFB:
  case 0xFC:
  case 0xFD:
  case 0xFE:
  case 0xFF:
    return 1;
  case 0xF0: // SysEx start
    return VARIABLE_LENGTH_SYSEX;
  default:
    return -1;
  }
}

inline void decodeMidiPacketBytes(
    const unsigned char *packetBytes, size_t packetLength,
    std::function<void(const unsigned char *message, size_t length)> destFn) {
  size_t i = 0;
  while (i < packetLength) {
    uint8_t status = packetBytes[i];
    int len = getMidiMessageLength(status);
    if (len == VARIABLE_LENGTH_SYSEX) {
      while (i < packetLength && packetBytes[i] != 0xF7)
        ++i;
      if (i < packetLength)
        ++i;
      continue;
    }
    if (len <= 0 || i + len > packetLength) {
      ++i; // safely skip 1 byte
      continue;
    }
    destFn(packetBytes + i, len);
    i += len;
  }
}

inline void debugDumpMidiBytes(const unsigned char *packetBytes,
                               size_t packetLength) {
  for (size_t i = 0; i < packetLength; i++) {
    printf("%02X ", packetBytes[i]);
  }
  printf("\n");
}

} // namespace vst_dev_host