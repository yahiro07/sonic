#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <string_view>

using Str = std::string_view;
using StrVec = const std::vector<std::string>&; 

class ParameterBuilder {
public:
  virtual ~ParameterBuilder() = default;
  virtual void addUnary(uint64_t address, Str identifier,
                        Str label, float defaultValue) = 0;
  virtual void addEnum(uint64_t address, Str identifier,
                       Str label, Str defaultValueString,
                       StrVec valueStrings) = 0;
  virtual void addBool(uint64_t address, Str identifier,
                       Str label, bool defaultValue) = 0;
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
