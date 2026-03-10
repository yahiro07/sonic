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

typedef struct _ParameterItem {
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
} ParameterItem;

} // namespace sonic
