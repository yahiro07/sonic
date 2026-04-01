#pragma once

#include "SynthesizerBase.hpp"

enum OscWaveType {
  Saw = 0,
  Square,
  Triangle,
  Sine,
};

class MySynthesizer : public SynthesizerBase {
public:
  MySynthesizer() {}
  ~MySynthesizer() override = default;

  void setupParameters(ParameterBuilder &builder) override;
  void prepareProcessing(double sampleRate, uint32_t maxFrameCount) override;

  void setParameter(uint32_t id, double value) override;
  void noteOn(int noteNumber, double velocity) override;
  void noteOff(int noteNumber) override;
  void processAudio(float *bufferL, float *bufferR, uint32_t frames) override;

private:
  float sampleRate = 0.0f;
  int noteNumber = 0;
  bool gateOn = false;

  bool oscEnabled = true;
  OscWaveType oscWave = OscWaveType::Saw;
  float oscPitch = 0.5f;
  float oscVolume = 0.5f;

  float phase = 0.0f;
};

SynthesizerBase *createSynthesizerInstance();