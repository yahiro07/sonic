#include "SynthesizerBase.hpp"

typedef struct _ParameterItem {
  uint64_t address;
  std::string identifier;
  std::string label;
  float defaultValue;
  float minValue;
  float maxValue;
  std::vector<std::string> valueStrings; // For enum parameters
} ParameterItem;

class ParameterBuilderImpl : ParameterBuilder {
  std::vector<ParameterItem> parameters;

public:
  std::vector<ParameterItem> getItems() { return parameters; }

  void addUnary(uint64_t address, Str identifier, Str label, float defaultValue) {
    parameters.push_back({
      .address = address,
      .identifier = std::string(identifier),
      .label = std::string(label),
      .defaultValue = defaultValue,
      .minValue = 0.0f,
      .maxValue = 1.0f,
      .valueStrings = {},
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
    });
  }
};
