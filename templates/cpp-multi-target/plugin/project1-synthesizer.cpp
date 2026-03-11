#include "project1-synthesizer.h"
#include <stdio.h>

namespace project1 {

void Project1Synthesizer::setupParameters(sonic::ParameterBuilder &builder) {
  builder.addUnary(0, "gain", "Gain", 0.1);
  builder.addEnum(1, "waveType", "Wave Type", "saw",
                  {"saw", "square", "triangle", "sine"});
  builder.addUnary(2, "oscPitch", "Pitch", 0.5);
  builder.addUnary(3, "oscVolume", "Volume", 0.5);
}

void Project1Synthesizer::setParameter(uint32_t id, float value) {
  if (id == 0) {
    paramGain = value;
  } else if (id == 1) {
    paramWaveType = value;
  } else if (id == 2) {
    paramOscPitch = value;
  } else if (id == 3) {
    paramOscVolume = value;
  }
}

void Project1Synthesizer::prepareProcessing(float sampleRate,
                                            uint32_t maxFrameCount) {
  this->sampleRate = sampleRate;
}

void Project1Synthesizer::processAudio(float *bufferL, float *bufferR,
                                       uint32_t frames) {
  if (sampleRate == 0.f)
    return;
  auto relNote = (paramOscPitch * 2.f - 1.f) * 12.f;
  auto note = noteNumber + relNote;
  auto freq = exp2f((note - 57.f) / 12.f) * 440.f;
  auto phaseInc = freq / sampleRate;

  auto gain = gateOn ? paramOscVolume : 0.f;
  // auto gain = .1f;

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

void Project1Synthesizer::noteOn(int noteNumber, float velocity) {
  this->noteNumber = noteNumber;
  this->gateOn = true;
}

void Project1Synthesizer::noteOff(int noteNumber) {
  if (noteNumber == this->noteNumber) {
    this->gateOn = false;
  }
}

void Project1Synthesizer::getDesiredEditorSize(uint32_t &width,
                                               uint32_t &height) {
  width = 800;
  height = 600;
}

std::string Project1Synthesizer::getEditorPageUrl() {
  return "http://localhost:3000";
}

} // namespace project1

sonic::SynthesizerBase *createSynthesizerInstance() {
  printf("project1-synthesizer: createSynthesizerInstance 0655\n");
  return new project1::Project1Synthesizer();
}
