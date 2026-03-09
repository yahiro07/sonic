#include "project1-synthesizer.h"
#include <stdio.h>

void Project1Synthesizer::setupParameters(
    sonic_common::ParameterBuilder &builder) {
  builder.addUnary(0, "gain", "Gain", 0.1);
  builder.addEnum(1, "waveType", "Wave Type", "saw",
                  {"saw", "square", "triangle", "sine"});
  builder.addUnary(2, "oscPitch", "Pitch", 0.5);
  builder.addUnary(3, "oscVolume", "Volume", 0.5);
}

void Project1Synthesizer::setParameter(uint64_t address, double value) {
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

void Project1Synthesizer::prepareProcessing(double sampleRate,
                                            uint32_t maxFrameCount) {
  this->sampleRate = (float)sampleRate;
}

void Project1Synthesizer::processAudio(float *bufferL, float *bufferR,
                                       uint32_t frames) {
  if (sampleRate == 0.f)
    return;
  auto freq = exp2f((noteNumber - 57.f) / 12.f) * 440.f;
  auto phaseInc = freq / sampleRate;

  auto gain = gateOn ? paramGain : 0.f;
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

void Project1Synthesizer::noteOn(int noteNumber, double velocity) {
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

sonic_common::SynthesizerBase *createSynthesizerInstance() {
  printf("project1-synthesizer: createSynthesizerInstance 0340\n");
  return new Project1Synthesizer();
}