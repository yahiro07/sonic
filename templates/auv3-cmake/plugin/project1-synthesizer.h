#include "../lib/common/synthesizer_base.h"
#include <math.h>

class Project1Synthesizer : public sonic_common::SynthesizerBase {
private:
  float sampleRate = 0.;
  float phase = 0.f;
  int noteNumber = 60;
  bool gateOn = false;

  float paramGain = 0.5f;
  float paramWaveType = 0.f;
  float paramOscPitch = 0.5f;
  float paramOscVolume = 0.5f;

public:
  void setupParameters(sonic_common::ParameterBuilder &builder) override;
  void setParameter(uint64_t address, double value) override;
  void prepareProcessing(double sampleRate, uint32_t maxFrameCount) override;
  void processAudio(float *bufferL, float *bufferR, uint32_t frames) override;
  void noteOn(int noteNumber, double velocity) override;
  void noteOff(int noteNumber) override;
  void getDesiredEditorSize(uint32_t &width, uint32_t &height) override;
};
