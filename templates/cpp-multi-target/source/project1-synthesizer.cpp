#include "project1-synthesizer.h"
#include <stdio.h>

namespace project1 {

enum ParameterAddress {
  kOscEnabled = 0,
  kOscWave,
  kOscPitch,
  kOscVolume,
};

void Project1Synthesizer::setupParameters(sonic::ParameterBuilder &builder) {
  builder.addBool(kOscEnabled, "oscEnabled", "Enabled", true);
  builder.addEnum(kOscWave, "oscWave", "Waveform", "saw",
                  {"saw", "square", "triangle", "sine"});
  builder.addUnary(kOscPitch, "oscPitch", "Pitch", 0.5);
  builder.addUnary(kOscVolume, "oscVolume", "Volume", 0.5);
}

void Project1Synthesizer::prepareProcessing(double sampleRate,
                                            uint32_t maxFrameCount) {
  this->sampleRate = sampleRate;
}

void Project1Synthesizer::setParameter(uint32_t id, double value) {
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

void Project1Synthesizer::noteOn(int noteNumber, double velocity) {
  this->noteNumber = noteNumber;
  this->gateOn = true;
}

void Project1Synthesizer::noteOff(int noteNumber) {
  if (noteNumber == this->noteNumber) {
    this->gateOn = false;
  }
}

void Project1Synthesizer::processAudio(float *bufferL, float *bufferR,
                                       uint32_t frames) {
  if (sampleRate <= 0.0f)
    return;

  if (0) {
    // debug filling white noise to check the audio path
    for (uint32_t i = 0; i < frames; ++i) {
      float y = (rand() / (float)RAND_MAX * 2.f - 1.f) * 0.1f;
      bufferL[i] = y;
      bufferR[i] = y;
    }
  }

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

void Project1Synthesizer::getDesiredEditorSize(uint32_t &width,
                                               uint32_t &height) {
  width = 800;
  height = 600;
}

std::string Project1Synthesizer::getEditorPageUrl() {
  if (0) {
    return "http://localhost:3000?debug=1";
  } else {
    return "app://www-bundles/index.html";
  }
}

} // namespace project1
