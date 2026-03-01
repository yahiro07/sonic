#include "audio_io_base.h"

#include <AudioToolbox/AudioToolbox.h>

class AudioIoMac : public AudioIoBase {
public:
  AudioIoMac();
  ~AudioIoMac();

  std::vector<AudioDeviceInfo> enumerateDevices() override;
  void open(const std::string &deviceKey, bool enableInput,
            void (*prepareFn)(double sampleRate, int maxFrameLength),
            void (*processFn)(float *bufferL, float *bufferR,
                              int nframes)) override;
  void close() override;

private:
  static OSStatus inputProc(void *inRefCon,
                            AudioUnitRenderActionFlags *ioActionFlags,
                            const AudioTimeStamp *inTimeStamp,
                            UInt32 inBusNumber, UInt32 inNumberFrames,
                            AudioBufferList *ioData);

  AudioUnit audioUnit = nullptr;
  bool isRunning = false;

  void (*prepareFn)(double sampleRate, int maxFrameLength) = nullptr;
  void (*processFn)(float *bufferL, float *bufferR, int nframes) = nullptr;

  std::vector<float> bufferL;
  std::vector<float> bufferR;
};