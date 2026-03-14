module;
#include "../api/synthesizer-base.h"

export module core:parameter_spec_item;
import std;

namespace sonic {

export using ParamId = uint32_t;

export enum class ParameterType {
  Unary,
  Enum,
  Bool,
};

export struct ParameterSpecItem {
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

export using ParameterSpecArray = std::vector<ParameterSpecItem>;

} // namespace sonic
