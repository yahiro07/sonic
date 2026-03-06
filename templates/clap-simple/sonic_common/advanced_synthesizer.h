#pragma once
#include "synthesizer_base.h"

class TelemetryBuilder {
public:
  // id must be within 0~31
  virtual void defineFloatArray(int id, uint32_t count) = 0;
  virtual void defineByteArray(int id, uint32_t count) = 0;
};

class AdvancedSynthesizer : public SynthesizerBase {
public:
  virtual ~AdvancedSynthesizer() = default;

  virtual void setupTelemetries(TelemetryBuilder &builder) {};
  // readTelemetry should be real-time safe and can be called from audio thread
  virtual bool readTelemetry(int id, float *buffer, uint32_t count) {
    return false;
  }
  virtual bool readTelemetry(int id, uint8_t *buffer, uint32_t count) {
    return false;
  }
};