#pragma once
#include <string>
#include <vector>

typedef struct {
  int deviceId;
  std::string name;
} MidiDeviceInfo;

class MidiInputManagerBase {
public:
  virtual std::vector<MidiDeviceInfo> enumerateDevices();
  virtual void
  open(int deviceId,
       void (*callback)(const std::vector<unsigned char> &message));
  virtual void close();
};