#include "SynthesizerBase.hpp"

using ParamId = uint32_t;

enum class CppParameterType {
  Unary,
  Enum,
  Bool,
};

struct CppParameterItem {
  ParamId id;
  std::string paramKey;
  std::string label;
  double defaultValue;
  double minValue;
  double maxValue;
  std::vector<std::string> valueStrings; // For enum parameters
  CppParameterType type;
  ParameterFlags flags;
};

class CppParameterBuilderImpl : public ParameterBuilder {
  std::vector<CppParameterItem> parameters;

public:
  std::vector<CppParameterItem> getItems() { return parameters; }

  void callSetupParameters(SynthesizerBase *synthInstance) {
    synthInstance->setupParameters(*this);
  }

  void addFloat(ParamId id, Str paramKey, Str label, double defaultValue,
                double minValue, double maxValue, Str group,
                ParameterFlags flags) override {
    parameters.push_back({
        .id = id,
        .paramKey = std::string(paramKey),
        .label = std::string(label),
        .defaultValue = defaultValue,
        .minValue = minValue,
        .maxValue = maxValue,
        .valueStrings = {},
        .type = CppParameterType::Unary,
        .flags = flags,
    });
  }

  void addEnum(ParamId id, Str paramKey, Str label, Str defaultValueString,
               StrVec valueStrings, ParameterFlags flags) override {
    parameters.push_back({
        .id = id,
        .paramKey = std::string(paramKey),
        .label = std::string(label),
        .defaultValue =
            find(valueStrings.begin(), valueStrings.end(),
                 defaultValueString) != valueStrings.end()
                ? (float)(std::distance(valueStrings.begin(),
                                        std::find(valueStrings.begin(),
                                                  valueStrings.end(),
                                                  defaultValueString)))
                : 0.0,
        .minValue = 0.0,
        .maxValue = (double)(valueStrings.size() - 1),
        .valueStrings =
            std::vector<std::string>(valueStrings.begin(), valueStrings.end()),
        .type = CppParameterType::Enum,
        .flags = flags,
    });
  }

  void addBool(ParamId id, Str paramKey, Str label, bool defaultValue,
               ParameterFlags flags) override {
    parameters.push_back({
        .id = id,
        .paramKey = std::string(paramKey),
        .label = std::string(label),
        .defaultValue = defaultValue ? 1.0 : 0.0,
        .minValue = 0.0,
        .maxValue = 1.0,
        .valueStrings = {},
        .type = CppParameterType::Bool,
        .flags = flags,
    });
  }
};
