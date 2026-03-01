#pragma once
#include "audio_io_base.h"
#include <AudioUnit/AudioUnit.h>
#include <functional>
#include <vector>

class AudioIoMac : public AudioIoBase {
public:
  AudioIoMac();
  ~AudioIoMac();

  std::vector<AudioDeviceInfo> enumerateDevices() override;
  void
  open(const std::string &deviceKey, bool enableInput,
       std::function<void(double sampleRate, int maxFrameLength)> prepareFn,
       std::function<void(float *bufferL, float *bufferR, int nframes)>
           processFn) override;
  void close() override;

private:
  static OSStatus inputProc(void *inRefCon,
                            AudioUnitRenderActionFlags *ioActionFlags,
                            const AudioTimeStamp *inTimeStamp,
                            UInt32 inBusNumber, UInt32 inNumberFrames,
                            AudioBufferList *ioData);

  AudioUnit audioUnit = nullptr;
  bool isRunning = false;

  std::function<void(double sampleRate, int maxFrameLength)> prepareFn =
      nullptr;
  std::function<void(float *bufferL, float *bufferR, int nframes)> processFn =
      nullptr;

  std::vector<float> bufferL;
  std::vector<float> bufferR;
};