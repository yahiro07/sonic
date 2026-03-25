#pragma once
#include <functional>
#include <string>
#include <vector>

namespace clap_dev_host {

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
       std::function<void(const unsigned char *message, size_t length)>
           callback) = 0;
  virtual void close() = 0;
};
} // namespace clap_dev_host