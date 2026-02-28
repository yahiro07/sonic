#include "MySynthesizer.h"
#include <cmath>

MySynthesizer::MySynthesizer() {}

enum ParameterAddress {
  kOscEnabled,
  kOscWave,
  kOscPitch,
  kOscVolume,
};

void MySynthesizer::setupParameters(ParameterBuilder &builder) {
  builder.addBool(kOscEnabled, "oscEnabled", "Osc Enabled", true);
  builder.addEnum(kOscWave, "oscWave", "Wave Type", "Saw",
                  {"Saw", "Square", "Triangle", "Sine"});
  builder.addUnary(kOscPitch, "oscPitch", "OSC Pitch", 0.5);
  builder.addUnary(kOscVolume, "oscVolume", "OSC Volume", 0.8);
}

void MySynthesizer::setParameter(uint32_t address, double value) {
  if (address == kOscEnabled) {
    oscEnabled = value;
  } else if (address == kOscWave) {
    oscWave = (OscWaveType)value;
  } else if (address == kOscPitch) {
    oscPitch = value;
  } else if (address == kOscVolume) {
    oscVolume = value;
  }
}

void MySynthesizer::prepare(double sampleRate, int32_t _maxFrameCount) {
  this->sampleRate = sampleRate;
}

void MySynthesizer::noteOn(int32_t noteNumber, double _velocity) {
  this->noteNumber = noteNumber;
  this->gateOn = true;
}

void MySynthesizer::noteOff(int32_t noteNumber) {
  if (noteNumber == this->noteNumber) {
    this->gateOn = false;
  }
}

void MySynthesizer::process(float *bufferL, float *bufferR, int32_t frames) {
  if (sampleRate <= 0.0f)
    return;

  float effectiveNoteNumber =
      (float)noteNumber + (oscPitch * 2.0f - 1.0f) * 12.0f;
  float frequency =
      440.0f * std::pow(2.0f, (effectiveNoteNumber - 69.0f) / 12.0f);
  float phaseDelta = frequency / sampleRate;

  float gain = (oscEnabled && gateOn) ? oscVolume : 0.0f;

  for (int32_t i = 0; i < frames; ++i) {
    phase += phaseDelta;
    if (phase >= 1.0f) {
      phase -= 1.0f;
    }
    float y = 0.0f;
    if (oscWave == OscWaveType::Saw) {
      y = (phase * 2.0f - 1.0f);
    } else if (oscWave == OscWaveType::Square) {
      y = (phase < 0.5f ? 1.0f : -1.0f);
    } else if (oscWave == OscWaveType::Triangle) {
      y = (phase < 0.5f ? 4.0f * phase - 1.0f : -4.0f * phase + 3.0f);
    } else if (oscWave == OscWaveType::Sine) {
      y = std::sin(phase * 2.0f * (float)M_PI);
    }
    bufferL[i] = y * gain;
  }
  memcpy(bufferR, bufferL, sizeof(float) * frames);
}

SynthesizerBase *createSynthesizerInstance() { return new MySynthesizer(); }