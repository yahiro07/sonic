#pragma once
#include <string>
#include <vector>

typedef struct {
  std::string deviceKey;
  std::string displayName;
} AudioDeviceInfo;

class AudioIoBase {
public:
  virtual ~AudioIoBase() = default;
  virtual std::vector<AudioDeviceInfo> enumerateDevices() = 0;
  virtual void open(const std::string &deviceKey, bool enableInput,
                    void (*prepareFn)(double sampleRate, int maxFrameLength),
                    void (*processFn)(float *bufferL, float *bufferR,
                                      int nframes)) = 0;
  virtual void close() = 0;
};