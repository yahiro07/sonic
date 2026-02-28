#pragma once
#include "../synthesizer_base.h"
#include <string>
#include <vector>

namespace vst3wf {

typedef uint32_t ParamAddress;

enum class ParameterType {
  Unary,
  Enum,
  Bool,
};

typedef struct _ParameterItem {
  ParamAddress address;
  std::string identifier;
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

} // namespace vst3wf
