#pragma once
#include "../api/synthesizer-base.h"
#include <string>
#include <vector>

namespace sonic {

using ParamId = uint32_t;

enum class ParameterType {
  Unary,
  Enum,
  Bool,
};

struct ParameterSpecItem {
  ParamId id;
  std::string paramKey;
  std::string label;
  float defaultValue;
  float minValue;
  float maxValue;
  std::vector<std::string> valueStrings; // For enum parameters
  ParameterType type;
  std::string group;
  ParameterFlags flags;
  // bool isInternal;
};

using ParameterSpecArray = std::vector<ParameterSpecItem>;

} // namespace sonic
