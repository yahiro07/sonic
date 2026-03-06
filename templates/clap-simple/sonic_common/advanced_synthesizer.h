#pragma once
#include "synthesizer_base.h"

class AdvancedSynthesizer : public SynthesizerBase {
public:
  virtual ~AdvancedSynthesizer() = default;
  void setupParameters(ParameterBuilder &builder) override {}
  void prepareProcessing(double sampleRate, uint32_t maxFrameCount) override {}
  void setParameter(uint64_t address, double value) override {}
  void processAudio(float *bufferL, float *bufferR, uint32_t frames) override {}
  void noteOn(int noteNumber, double velocity) override {}
  void noteOff(int noteNumber) override {}
  std::string getEditorPageUrl() override { return "http://localhost:3000"; }
};