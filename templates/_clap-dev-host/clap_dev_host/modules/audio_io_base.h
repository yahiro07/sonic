#pragma once
#include <functional>
#include <string>
#include <vector>

namespace clap_dev_host {

typedef struct {
  std::string deviceKey;
  std::string displayName;
} AudioDeviceInfo;

class AudioIoBase {
public:
  virtual ~AudioIoBase() = default;
  virtual std::vector<AudioDeviceInfo> enumerateDevices() = 0;
  virtual void
  open(const std::string &deviceKey, bool enableInput,
       std::function<void(double sampleRate, int maxFrameLength)> prepareFn,
       std::function<void(float *bufferL, float *bufferR, int nframes)>
           processFn) = 0;
  virtual void close() = 0;
};

} // namespace clap_dev_host