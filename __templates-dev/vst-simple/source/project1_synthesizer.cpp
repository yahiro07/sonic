#include "project1_synthesizer.h"
#include <cmath>

Project1Synthesizer::Project1Synthesizer() {}

enum ParameterAddress {
  kOscEnabled = 0,
  kOscWave,
  kOscPitch,
  kOscVolume,
};

void Project1Synthesizer::setupParameters(ParameterBuilder &builder) {
  builder.addBool(kOscEnabled, "oscEnabled", "Osc Enabled", true);
  builder.addEnum(kOscWave, "oscWave", "Wave Type", "Saw",
                  {"Saw", "Square", "Triangle", "Sine"});
  builder.addFloat(kOscPitch, "oscPitch", "OSC Pitch", 0.5);
  builder.addFloat(kOscVolume, "oscVolume", "OSC Volume", 0.8);
}

void Project1Synthesizer::setParameter(uint64_t address, double value) {
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

void Project1Synthesizer::prepare(double sampleRate, int32_t _maxFrameCount) {
  this->sampleRate = sampleRate;
}

void Project1Synthesizer::noteOn(int32_t noteNumber, double _velocity) {
  this->noteNumber = noteNumber;
  this->gateOn = true;
}

void Project1Synthesizer::noteOff(int32_t noteNumber) {
  if (noteNumber == this->noteNumber) {
    this->gateOn = false;
  }
}

void Project1Synthesizer::process(float *bufferL, float *bufferR,
                                  int32_t frames) {
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

std::string Project1Synthesizer::getEditorPageUrl() {
  if (0) {
    return "http://localhost:3000?debug=1";
  } else {
    return "app://local/index.html?debug=1";
    // app://local is mapped to the resources/www directory
    // return "app://local/index.html";
  }
}

SynthesizerBase *createSynthesizerInstance() {
  return new Project1Synthesizer();
}