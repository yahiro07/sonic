#include "sonic_common/synthesizer_base.h"
#include <math.h>

class MySynthesizer : public SynthesizerBase {
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
  void setupParameters(ParameterBuilder &builder) override {
    builder.addUnary(0, "gain", "Gain", 0.5);
    builder.addEnum(1, "waveType", "Wave Type", "saw",
                    {"saw", "square", "triangle", "sine"});
    builder.addUnary(2, "oscPitch", "Pitch", 0.5);
    builder.addUnary(3, "oscVolume", "Volume", 0.5);
  }

  void setParameter(uint64_t address, double value) override {
    if (address == 0) {
      paramGain = value;
    } else if (address == 1) {
      paramWaveType = value;
    } else if (address == 2) {
      paramOscPitch = value;
    } else if (address == 3) {
      paramOscVolume = value;
    }
  }

  void setSampleRate(double sampleRate) override {
    this->sampleRate = (float)sampleRate;
  }
  void processAudio(float *bufferL, float *bufferR, int32_t frames) override {
    if (sampleRate == 0.f)
      return;
    auto freq = exp2f((noteNumber - 57.f) / 12.f) * 440.f;
    auto gain = gateOn ? paramGain : 0.f;
    auto phaseInc = freq / sampleRate;

    for (uint32_t index = 0; index < frames; index++) {
      phase += phaseInc;
      phase -= floorf(phase);
      // auto y = rand() / (float)RAND_MAX * 2.f - 1.f;
      auto y = (phase * 2.f - 1.f);
      // auto y = sinf(phase * 2.f * M_PI);
      y *= gain;
      bufferL[index] = y;
      bufferR[index] = y;
    }
  }
  void noteOn(int noteNumber, double velocity) override {
    this->noteNumber = noteNumber;
    this->gateOn = true;
  }
  void noteOff(int noteNumber) override {
    if (noteNumber == this->noteNumber) {
      this->gateOn = false;
    }
  }

  std::string_view getEditorPageUrl() override {
    return "http://localhost:3000";
  }
};

inline SynthesizerBase *createSynthesizerInstance() {
  return new MySynthesizer();
}
