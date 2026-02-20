#pragma once

#include <cstdint>
#include <string>
#include <vector>

class ParameterBuilder {
public:
  virtual ~ParameterBuilder() = default;
  virtual void addUnary(uint64_t address, const std::string &identifier,
                        const std::string &label, float defaultValue) = 0;
  virtual void addEnum(uint64_t address, const std::string &identifier,
                       const std::string &label, float defaultValue,
                       const std::vector<std::string> &valueStrings) = 0;
  virtual void addBool(uint64_t address, const std::string &identifier,
                       const std::string &label, bool defaultValue) = 0;
};

class SynthesizerBase {
public:
  virtual ~SynthesizerBase() = default;
  virtual void setupParameters(ParameterBuilder &builder) = 0;
  virtual void setParameter(uint64_t address, float value) = 0;
  virtual void prepare(float sampleRate, int32_t maxFrameCount) = 0;
  virtual void noteOn(int32_t noteNumber, float velocity) = 0;
  virtual void noteOff(int32_t noteNumber) = 0;
  virtual void process(float *buffer, int32_t frames) = 0;
};
