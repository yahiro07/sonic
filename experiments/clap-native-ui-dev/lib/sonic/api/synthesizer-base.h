#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace sonic {

enum ParameterFlags : int {
  None = 0,
  IsReadOnly = 1 << 0,
  IsHidden = 1 << 1,
  NonAutomatable = 1 << 2,
};
static ParameterFlags operator|(ParameterFlags a, ParameterFlags b) {
  return static_cast<ParameterFlags>(static_cast<int>(a) | static_cast<int>(b));
}

class ParameterBuilder {
protected:
  using Str = std::string_view;
  using StrVec = const std::vector<std::string_view> &;

public:
  virtual ~ParameterBuilder() = default;
  virtual void addUnary(uint32_t id, Str paramKey, Str label,
                        double defaultValue, Str group = "",
                        ParameterFlags flags = ParameterFlags::None) = 0;
  virtual void addEnum(uint32_t id, Str paramKey, Str label,
                       Str defaultValueString, StrVec valueStrings,
                       Str group = "",
                       ParameterFlags flags = ParameterFlags::None) = 0;
  virtual void addBool(uint32_t id, Str paramKey, Str label, bool defaultValue,
                       Str group = "",
                       ParameterFlags flags = ParameterFlags::None) = 0;
};

class SynthesizerBase {
public:
  virtual ~SynthesizerBase() = default;
  virtual void setupParameters(ParameterBuilder &builder) = 0;
  virtual void setParameter(uint32_t id, double value) = 0;
  virtual void prepareProcessing(double sampleRate, uint32_t maxFrameCount) = 0;
  virtual void processAudio(float *bufferL, float *bufferR,
                            uint32_t frames) = 0;
  virtual void noteOn(int noteNumber, double velocity) = 0;
  virtual void noteOff(int noteNumber) = 0;

  virtual void getDesiredEditorSize(uint32_t &width, uint32_t &height) = 0;
  virtual std::string getEditorPageUrl() = 0;
};

} // namespace sonic

sonic::SynthesizerBase *createSynthesizerInstance();
