#pragma once
#include <functional>
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
       std::function<void(const std::vector<unsigned char> &message)>
           callback) = 0;
  virtual void close() = 0;
};