#include "MySynthesizer.h"
#include <cmath>

MySynthesizer::MySynthesizer() {}

void MySynthesizer::setupParameters(ParameterBuilder &builder) {
  builder.addUnary(0, "gain", "gain", 0.5);
  builder.addUnary(1, "oscPitch", "OSC Pitch", 0.5);
  builder.addUnary(2, "oscVolume", "OSC Volume", 0.5);
}

void MySynthesizer::setParameter(uint64_t address, double value) {
  switch (address) {
  case 1:
    oscPitch = value;
    break;
  case 2:
    oscVolume = value;
    break;
  }
}

void MySynthesizer::prepare(double sampleRate, int32_t _maxFrameCount) {
  this->sampleRate = sampleRate;
}

void MySynthesizer::noteOn(int32_t noteNumber, double _velocity) {
  this->noteNumber = noteNumber;
  this->gateOn = true;
}

void MySynthesizer::noteOff(int32_t _noteNumber) { this->gateOn = false; }

void MySynthesizer::process(float *bufferL, float *bufferR, int32_t frames) {
  if (sampleRate <= 0.0f)
    return;

  float effectiveNoteNumber =
      (float)noteNumber + (oscPitch * 2.0f - 1.0f) * 12.0f;
  float frequency =
      440.0f * std::pow(2.0f, (effectiveNoteNumber - 69.0f) / 12.0f);
  float phaseDelta = frequency / sampleRate;

  for (int32_t i = 0; i < frames; ++i) {
    phase += phaseDelta;
    if (phase >= 1.0f) {
      phase -= 1.0f;
    }

    float y = 0.0f;
    if (gateOn) {
      y = std::sin(phase * 2.0f * (float)M_PI) * oscVolume;
    }
    bufferL[i] = y;
  }
  memcpy(bufferR, bufferL, sizeof(float) * frames);
}

SynthesizerBase *createSynthesizerInstance() { return new MySynthesizer(); }