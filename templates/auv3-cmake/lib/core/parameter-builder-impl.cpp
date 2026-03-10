#include "./parameter-builder-impl.h"

namespace sonic {

void ParameterBuilderImpl::addUnary(uint32_t id, Str paramKey, Str label,
                                    float defaultValue, Str group,
                                    ParameterFlags flags) {
  parameters.push_back({
      .id = id,
      .paramKey = std::string(paramKey),
      .label = std::string(label),
      .defaultValue = defaultValue,
      .minValue = 0.f,
      .maxValue = 1.f,
      .valueStrings = {},
      .type = ParameterType::Unary,
      .group = std::string(group),
      .flags = flags,
  });
}

void ParameterBuilderImpl::addEnum(uint32_t id, Str paramKey, Str label,
                                   Str defaultValueString, StrVec valueStrings,
                                   Str group, ParameterFlags flags) {
  const float maxValue =
      valueStrings.empty() ? 0.f : static_cast<float>(valueStrings.size() - 1);
  parameters.push_back({
      .id = id,
      .paramKey = std::string(paramKey),
      .label = std::string(label),
      .defaultValue =
          find(valueStrings.begin(), valueStrings.end(), defaultValueString) !=
                  valueStrings.end()
              ? (float)(std::distance(valueStrings.begin(),
                                      std::find(valueStrings.begin(),
                                                valueStrings.end(),
                                                defaultValueString)))
              : 0.f,
      .minValue = 0.f,
      .maxValue = maxValue,
      .valueStrings =
          std::vector<std::string>(valueStrings.begin(), valueStrings.end()),
      .type = ParameterType::Enum,
      .group = std::string(group),
      .flags = flags,
  });
}

void ParameterBuilderImpl::addBool(uint32_t id, Str paramKey, Str label,
                                   bool defaultValue, Str group,
                                   ParameterFlags flags) {
  parameters.push_back({
      .id = id,
      .paramKey = std::string(paramKey),
      .label = std::string(label),
      .defaultValue = defaultValue ? 1.f : 0.f,
      .minValue = 0.f,
      .maxValue = 1.f,
      .valueStrings = {},
      .type = ParameterType::Bool,
      .group = std::string(group),
      .flags = flags,
  });
}

} // namespace sonic