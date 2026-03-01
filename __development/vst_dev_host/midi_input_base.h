#pragma once
#include <string>
#include <vector>

typedef struct {
  std::string deviceKey;
  std::string displayName;
} MidiDeviceInfo;

class MidiInputBase {
public:
  virtual ~MidiInputBase() = default;
  virtual std::vector<MidiDeviceInfo> enumerateDevices() = 0;
  virtual void
  open(const std::string &deviceKey,
       void (*callback)(const std::vector<unsigned char> &message)) = 0;
  virtual void close() = 0;
};