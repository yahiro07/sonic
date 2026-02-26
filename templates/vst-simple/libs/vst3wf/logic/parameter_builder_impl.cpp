#include "./parameter_builder_impl.h"
#include "../SynthesizerBase.h"

void ParameterBuilderImpl::addUnary(uint32_t address, Str identifier, Str label,
                                    double defaultValue, Str group,
                                    ParameterFlags flags) {
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
  });
}

void ParameterBuilderImpl::addEnum(uint32_t address, Str identifier, Str label,
                                   Str defaultValueString, StrVec valueStrings,
                                   Str group, ParameterFlags flags) {
  const double maxValue =
      valueStrings.empty() ? 0.0 : static_cast<double>(valueStrings.size() - 1);
  parameters.push_back({
      .address = address,
      .identifier = std::string(identifier),
      .label = std::string(label),
      .defaultValue =
          find(valueStrings.begin(), valueStrings.end(), defaultValueString) !=
                  valueStrings.end()
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
  });
}

void ParameterBuilderImpl::addBool(uint32_t address, Str identifier, Str label,
                                   bool defaultValue, Str group,
                                   ParameterFlags flags) {
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
  });
}