#pragma once
#include "./parameter_item.h"

namespace sonic_common {

class ParameterBuilderImpl : public ParameterBuilder {
private:
  std::vector<ParameterItem> parameters;

public:
  std::vector<ParameterItem> getItems() { return parameters; }
  void callSetupParameters(SynthesizerBase *synthInstance) {
    synthInstance->setupParameters(*this);
  }
  void addUnary(uint64_t address, Str identifier, Str label,
                double defaultValue, Str group, ParameterFlags flags);
  void addEnum(uint64_t address, Str identifier, Str label,
               Str defaultValueString, StrVec valueStrings, Str group,
               ParameterFlags flags);
  void addBool(uint64_t address, Str identifier, Str label, bool defaultValue,
               Str group, ParameterFlags flags);
};

} // namespace sonic_common