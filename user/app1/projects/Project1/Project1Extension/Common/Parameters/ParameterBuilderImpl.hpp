#include "SynthesizerBase.hpp"

enum class ParameterType {
  Unary,
  Enum,
  Bool,
};

typedef struct _ParameterItem {
  uint64_t address;
  std::string identifier;
  std::string label;
  float defaultValue;
  float minValue;
  float maxValue;
  std::vector<std::string> valueStrings; // For enum parameters
  ParameterType type;
} ParameterItem;

class ParameterBuilderImpl : public ParameterBuilder {
  std::vector<ParameterItem> parameters;

public:
  std::vector<ParameterItem> getItems() { return parameters; }

  void callSetupParameters(SynthesizerBase *synthInstance) {
    synthInstance->setupParameters(*this);
  }

  void addUnary(uint64_t address, Str identifier, Str label, float defaultValue) {
    parameters.push_back({
      .address = address,
      .identifier = std::string(identifier),
      .label = std::string(label),
      .defaultValue = defaultValue,
      .minValue = 0.0f,
      .maxValue = 1.0f,
      .valueStrings = {},
      .type = ParameterType::Unary,
    });
  }

  void addEnum(uint64_t address, Str identifier, Str label, Str defaultValueString,
               StrVec valueStrings) {
    parameters.push_back({
      .address = address,
      .identifier = std::string(identifier),
      .label = std::string(label),
      .defaultValue =
        find(valueStrings.begin(), valueStrings.end(), defaultValueString) !=
            valueStrings.end()
          ? (float)(std::distance(
              valueStrings.begin(),
              std::find(valueStrings.begin(), valueStrings.end(), defaultValueString)))
          : 0.0f,
      .minValue = 0.0f,
      .maxValue = (float)(valueStrings.size() - 1),
      .valueStrings = std::vector<std::string>(valueStrings.begin(), valueStrings.end()),
      .type = ParameterType::Enum,
    });
  }

  void addBool(uint64_t address, Str identifier, Str label, bool defaultValue) {
    parameters.push_back({
      .address = address,
      .identifier = std::string(identifier),
      .label = std::string(label),
      .defaultValue = defaultValue ? 1.0f : 0.0f,
      .minValue = 0.0f,
      .maxValue = 1.0f,
      .valueStrings = {},
      .type = ParameterType::Bool,
    });
  }
};
