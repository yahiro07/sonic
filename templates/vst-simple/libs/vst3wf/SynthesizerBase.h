#pragma once

#include <cstdint>
// #include <map>
#include <string>
#include <string_view>
#include <vector>

enum ParameterFlags : int {
  None = 0,
  IsReadOnly = 1,
  IsHidden = 2,
  // IsInternal = 4
};

class ParameterBuilder {
protected:
  using Str = std::string_view;
  using StrVec = const std::vector<std::string> &;

public:
  virtual ~ParameterBuilder() = default;
  virtual void addUnary(uint32_t address, Str identifier, Str label,
                        double defaultValue, Str group = "",
                        ParameterFlags flags = ParameterFlags::None) = 0;
  virtual void addEnum(uint32_t address, Str identifier, Str label,
                       Str defaultValueString, StrVec valueStrings,
                       Str group = "",
                       ParameterFlags flags = ParameterFlags::None) = 0;
  // virtual void addEnum(uint32_t address, Str identifier, Str label,
  //                      int defaultKey, std::map<int, Str> valueOptions) = 0;
  virtual void addBool(uint32_t address, Str identifier, Str label,
                       bool defaultValue, Str group = "",
                       ParameterFlags flags = ParameterFlags::None) = 0;
};

class SynthesizerBase {
public:
  virtual ~SynthesizerBase() = default;
  virtual void setupParameters(ParameterBuilder &builder) = 0;
  virtual void setParameter(uint32_t address, double value) = 0;
  virtual void prepare(double sampleRate, int32_t maxFrameCount) = 0;
  virtual void noteOn(int32_t noteNumber, double velocity) = 0;
  virtual void noteOff(int32_t noteNumber) = 0;
  virtual void process(float *bufferL, float *bufferR, int32_t frames) = 0;
};
