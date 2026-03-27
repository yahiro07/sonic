#include "MySynthesizer.hpp"
#include <cmath>

enum ParameterAddress {
  kOscEnabled = 0,
  kOscWave,
  kOscPitch,
  kOscVolume,
};

void MySynthesizer::setupParameters(ParameterBuilder &builder) {
  builder.addBool(kOscEnabled, "oscEnabled", "Enabled", true);
  builder.addEnum(kOscWave, "oscWave", "Waveform", "saw",
                  {"saw", "square", "triangle", "sine"});
  builder.addUnary(kOscPitch, "oscPitch", "Pitch", 0.5);
  builder.addUnary(kOscVolume, "oscVolume", "Volume", 0.5);
}

void MySynthesizer::prepareProcessing(double sampleRate,
                                      uint32_t _maxFrameCount) {
  this->sampleRate = sampleRate;
}

void MySynthesizer::setParameter(uint32_t id, double value) {
  if (id == kOscEnabled) {
    oscEnabled = value;
  } else if (id == kOscWave) {
    oscWave = (OscWaveType)value;
  } else if (id == kOscPitch) {
    oscPitch = value;
  } else if (id == kOscVolume) {
    oscVolume = value;
  }
}

void MySynthesizer::noteOn(int noteNumber, double velocity) {
  this->noteNumber = noteNumber;
  this->gateOn = true;
}

void MySynthesizer::noteOff(int noteNumber) {
  if (noteNumber == this->noteNumber) {
    this->gateOn = false;
  }
}

void MySynthesizer::processAudio(float *bufferL, float *bufferR,
                                 uint32_t frames) {
  if (sampleRate <= 0.0f)
    return;

  float effectiveNoteNumber =
      (float)noteNumber + (oscPitch * 2.0f - 1.0f) * 12.0f;
  float frequency = 440.0f * pow(2.0f, (effectiveNoteNumber - 69.0f) / 12.0f);
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
      y = sin(phase * 2.0f * (float)M_PI);
    }
    bufferL[i] = y * gain;
  }
  memcpy(bufferR, bufferL, sizeof(float) * frames);
}

SynthesizerBase *createSynthesizerInstance() { return new MySynthesizer(); }