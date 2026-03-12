#pragma once
#include "./parameter_item.h"

namespace sonic_vst {

class ParameterBuilderImpl : public ParameterBuilder {
private:
  std::vector<ParameterItem> parameters;

public:
  std::vector<ParameterItem> getItems() { return parameters; }
  void callSetupParameters(SynthesizerBase *synthInstance) {
    synthInstance->setupParameters(*this);
  }
  void addUnary(uint32_t id, Str paramKey, Str label, float defaultValue,
                Str group, ParameterFlags flags) override;
  void addEnum(uint32_t id, Str paramKey, Str label, Str defaultValueString,
               StrVec valueStrings, Str group, ParameterFlags flags) override;
  void addBool(uint32_t id, Str paramKey, Str label, bool defaultValue,
               Str group, ParameterFlags flags) override;
};

} // namespace sonic_vst