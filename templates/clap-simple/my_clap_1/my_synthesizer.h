#include "./synthesizer_base.h"
#include <cstdio>
#include <math.h>

class MySynthesizer : public SynthesizerBase {
private:
  float sampleRate = 0.;
  float phase = 0.f;
  int noteNumber = 60;
  bool gateOn = false;

  float paramGain = 0.5f;

public:
  void setSampleRate(double sampleRate) override {
    this->sampleRate = (float)sampleRate;
  }
  void processAudio(float *bufferL, float *bufferR, uint32_t frames) override {
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

  uint32_t getParameterCount() const override { return 1; }

  void getParameterInfo(uint32_t index,
                        clap_param_info_t *info) const override {
    info->id = 0;
    info->flags = CLAP_PARAM_IS_AUTOMATABLE;
    info->min_value = 0.0;
    info->max_value = 1.0;
    info->default_value = 0.5;
    snprintf(info->name, sizeof(info->name), "Gain");
  }

  double getParameterValue(clap_id id) const override { return paramGain; }

  void setParameterValue(clap_id id, double value) override {
    paramGain = value;
  }
};

inline SynthesizerBase *createSynthesizerInstance() {
  return new MySynthesizer();
}
