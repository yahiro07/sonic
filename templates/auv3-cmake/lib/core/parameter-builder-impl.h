#pragma once
#include "./parameter-spec-item.h"

namespace sonic {

class ParameterBuilderImpl : public ParameterBuilder {
private:
  std::vector<ParameterSpecItem> parameters;

public:
  std::vector<ParameterSpecItem> getItems() { return parameters; }

  void addUnary(uint32_t id, Str paramKey, Str label, float defaultValue,
                Str group, ParameterFlags flags);
  void addEnum(uint32_t id, Str paramKey, Str label, Str defaultValueString,
               StrVec valueStrings, Str group, ParameterFlags flags);
  void addBool(uint32_t id, Str paramKey, Str label, bool defaultValue,
               Str group, ParameterFlags flags);
};

} // namespace sonic