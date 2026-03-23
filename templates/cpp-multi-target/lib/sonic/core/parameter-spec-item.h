#pragma once
#include "./types.h"

namespace sonic {

enum class ParameterType {
  Unary,
  Enum,
  Bool,
};

struct ParameterSpecItem {
  ParamId id;
  std::string paramKey;
  std::string label;
  double defaultValue;
  double minValue;
  double maxValue;
  std::vector<std::string> valueStrings; // For enum parameters
  ParameterType type;
  std::string group;
  ParameterFlags flags;
  // bool isInternal;
};

using ParameterSpecArray = std::vector<ParameterSpecItem>;

} // namespace sonic
