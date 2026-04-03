#pragma once

#include "sonic/synthesizer_base.h"

enum OscWaveType {
  Saw = 0,
  Square,
  Triangle,
  Sine,
};

class Project1Synthesizer : public SynthesizerBase {
public:
  Project1Synthesizer();
  ~Project1Synthesizer() override = default;

  void setupParameters(ParameterBuilder &builder) override;
  void setParameter(uint64_t address, double value) override;
  void prepare(double sampleRate, int32_t maxFrameCount) override;
  void noteOn(int32_t noteNumber, double velocity) override;
  void noteOff(int32_t noteNumber) override;
  void process(float *bufferL, float *bufferR, int32_t frames) override;
  std::string getEditorPageUrl() override;

private:
  bool oscEnabled = true;
  OscWaveType oscWave = OscWaveType::Saw;
  float oscPitch = 0.5f;
  float oscVolume = 0.5f;
  int32_t noteNumber = 60;
  bool gateOn = false;
  float sampleRate = 0.0f;
  float phase = 0.0f;
};

SynthesizerBase *createSynthesizerInstance();