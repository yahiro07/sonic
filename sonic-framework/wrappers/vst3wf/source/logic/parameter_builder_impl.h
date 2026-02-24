#pragma once

#include "../dsp/SynthesizerBase.h"

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

class ParameterBuilderImpl : public ParameterBuilder {
  std::vector<ParameterItem> parameters;

public:
  std::vector<ParameterItem> getItems() { return parameters; }

  void callSetupParameters(SynthesizerBase *synthInstance) {
    synthInstance->setupParameters(*this);
  }

  void addUnary(uint32_t address, Str identifier, Str label,
                double defaultValue, Str group, ParameterFlags flags) {
    parameters.push_back({
        .address = address,
        .identifier = std::string(identifier),
        .label = std::string(label),
        .defaultValue = defaultValue,
        .minValue = 0.0,
        .maxValue = 1.0,
        .valueStrings = {},
        .type = ParameterType::Unary,
        .group = std::string(group),
        .flags = flags,
        .isInternal = (flags & ParameterFlags::IsInternal) > 0,
    });
  }

  void addEnum(uint32_t address, Str identifier, Str label,
               Str defaultValueString, StrVec valueStrings, Str group,
               ParameterFlags flags) {
    const double maxValue = valueStrings.empty()
                                ? 0.0
                                : static_cast<double>(valueStrings.size() - 1);
    parameters.push_back({
        .address = address,
        .identifier = std::string(identifier),
        .label = std::string(label),
        .defaultValue =
            find(valueStrings.begin(), valueStrings.end(),
                 defaultValueString) != valueStrings.end()
                ? (float)(std::distance(valueStrings.begin(),
                                        std::find(valueStrings.begin(),
                                                  valueStrings.end(),
                                                  defaultValueString)))
                : 0.0f,
        .minValue = 0.0,
        .maxValue = maxValue,
        .valueStrings =
            std::vector<std::string>(valueStrings.begin(), valueStrings.end()),
        .type = ParameterType::Enum,
        .group = std::string(group),
        .flags = flags,
        .isInternal = (flags & ParameterFlags::IsInternal) > 0,
    });
  }

  void addBool(uint32_t address, Str identifier, Str label, bool defaultValue,
               Str group, ParameterFlags flags) {
    parameters.push_back({
        .address = address,
        .identifier = std::string(identifier),
        .label = std::string(label),
        .defaultValue = defaultValue ? 1.0 : 0.0,
        .minValue = 0.0,
        .maxValue = 1.0,
        .valueStrings = {},
        .type = ParameterType::Bool,
        .group = std::string(group),
        .flags = flags,
        .isInternal = (flags & ParameterFlags::IsInternal) > 0,
    });
  }
};
