#pragma once
#include "../SynthesizerBase.h"
#include <string>
#include <vector>

enum class ParameterType {
  Unary,
  Enum,
  Bool,
};

typedef struct _ParameterItem {
  uint32_t address;
  std::string identifier;
  std::string label;
  double defaultValue;
  double minValue;
  double maxValue;
  std::vector<std::string> valueStrings; // For enum parameters
  ParameterType type;
  std::string group;
  ParameterFlags flags;
  bool isInternal;
} ParameterItem;
