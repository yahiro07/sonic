module;
#include "../api/synthesizer-base.h"

export module core:parameter_builder_impl;
import std;
import :parameter_spec_item;

namespace sonic {

export class ParameterBuilderImpl : public ParameterBuilder {
private:
  std::vector<ParameterSpecItem> parameters;

public:
  std::vector<ParameterSpecItem> getItems() { return parameters; }

  void addUnary(uint32_t id, Str paramKey, Str label, double defaultValue,
                Str group, ParameterFlags flags) override {
    parameters.push_back({
        .id = id,
        .paramKey = std::string(paramKey),
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
  void addEnum(uint32_t id, Str paramKey, Str label, Str defaultValueString,
               StrVec valueStrings, Str group, ParameterFlags flags) override {
    const double maxValue = valueStrings.empty()
                                ? 0.0
                                : static_cast<double>(valueStrings.size() - 1);
    parameters.push_back({
        .id = id,
        .paramKey = std::string(paramKey),
        .label = std::string(label),
        .defaultValue =
            std::find(valueStrings.begin(), valueStrings.end(),
                      defaultValueString) != valueStrings.end()
                ? (double)(std::distance(valueStrings.begin(),
                                         std::find(valueStrings.begin(),
                                                   valueStrings.end(),
                                                   defaultValueString)))
                : 0.0,
        .minValue = 0.0,
        .maxValue = maxValue,
        .valueStrings =
            std::vector<std::string>(valueStrings.begin(), valueStrings.end()),
        .type = ParameterType::Enum,
        .group = std::string(group),
        .flags = flags,
    });
  }
  void addBool(uint32_t id, Str paramKey, Str label, bool defaultValue,
               Str group, ParameterFlags flags) override {
    parameters.push_back({
        .id = id,
        .paramKey = std::string(paramKey),
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
};

} // namespace sonic