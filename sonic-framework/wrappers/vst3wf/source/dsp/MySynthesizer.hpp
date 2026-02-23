#pragma once

#include "SynthesizerBase.hpp"

class MySynthesizer : public SynthesizerBase {
public:
  MySynthesizer();
  ~MySynthesizer() override = default;

  void setupParameters(ParameterBuilder &builder) override;
  void setParameter(uint64_t address, float value) override;
  void prepare(float sampleRate, int32_t maxFrameCount) override;
  void noteOn(int32_t noteNumber, float velocity) override;
  void noteOff(int32_t noteNumber) override;
  void process(float *bufferL, float *bufferR, int32_t frames) override;

private:
  float oscPitch = 0.5f;
  float oscVolume = 0.5f;
  int32_t noteNumber = 60;
  bool gateOn = false;
  float sampleRate = 0.0f;
  float phase = 0.0f;
};

SynthesizerBase *createSynthesizerInstance();