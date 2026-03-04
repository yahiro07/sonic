#include "clap_wrapper.h"
#include <math.h>

class MySynthesizer : public SynthesizerBase {
private:
  float sampleRate = 0.;
  float phase = 0.f;
  int noteNumber = 60;
  bool gateOn = false;

public:
  void setSampleRate(double sampleRate) {
    this->sampleRate = (float)sampleRate;
  }
  void processAudio(float *bufferL, float *bufferR, uint32_t frames) {
    if (sampleRate == 0.f)
      return;
    auto freq = exp2f((noteNumber - 57.f) / 12.f) * 440.f;
    auto gain = gateOn ? .5f : 0.f;
    auto phaseInc = freq / sampleRate;

    for (uint32_t index = 0; index < frames; index++) {
      phase += phaseInc;
      phase -= floorf(phase);
      // auto y = rand() / (float)RAND_MAX * 2.f - 1.f;
      // auto y = (phase * 2.f - 1.f);
      auto y = sinf(phase * 2.f * M_PI);
      y *= gain;
      bufferL[index] = y;
      bufferR[index] = y;
    }
  }
  void noteOn(int noteNumber, double velocity) {
    this->noteNumber = noteNumber;
    this->gateOn = true;
  }
  void noteOff(int noteNumber) {
    if (noteNumber == this->noteNumber) {
      this->gateOn = false;
    }
  }
};

SynthesizerBase *createSynthesizerInstance() { return new MySynthesizer(); }

PluginMeta meta = {
    .id = "com.my-company.my-plugin",
    .name = "MyPlugin",
    .vendor = "MyCompany",
    .url = "https://my-company.com",
    .manualUrl = "https://my-company.com/manual",
    .supportUrl = "https://my-company.com/support",
    .version = "1.0.0",
    .description = "Example CLAP plugin.",
};

extern "C" const clap_plugin_entry_t clap_entry =
    createClapPluginEntry(createSynthesizerInstance, meta);
