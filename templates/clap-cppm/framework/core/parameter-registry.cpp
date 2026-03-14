#include "./parameter-registry.h"
#include <optional>
#include <string>
#include <unordered_map>

namespace sonic {

void ParameterRegistry::addParameters(ParameterSpecArray &parameterItems,
                                      uint32_t maxId) {
  this->parameterItems.clear();
  this->parameterItemsMap.clear();
  this->paramKeyToIdMap.clear();
  for (const auto &item : parameterItems) {
    auto validInRange = item.id <= maxId;
    if (!validInRange) {
      printf("ParametersManager: id out of range for ParamKey: %llu "
             "(%s)\n",
             static_cast<unsigned long long>(item.id), item.paramKey.c_str());
      continue;
    }
    auto id = item.id;
    this->parameterItems.push_back(item);
  }
  for (auto &item : this->parameterItems) {
    this->parameterItemsMap[item.id] = &item;
    this->paramKeyToIdMap[item.paramKey] = item.id;
  }
}

std::optional<ParamId>
ParameterRegistry::getIdByParamKey(const std::string &paramKey) {
  auto val = paramKeyToIdMap.find(paramKey);
  if (val == paramKeyToIdMap.end()) {
    return std::nullopt;
  }
  return val->second;
}

std::optional<std::string> ParameterRegistry::getParamKeyById(ParamId id) {
  auto val = parameterItemsMap.find(id);
  if (val == parameterItemsMap.end()) {
    return std::nullopt;
  }
  return val->second->paramKey;
}

const ParameterSpecItem *ParameterRegistry::getParameterItemById(ParamId id) {
  auto val = parameterItemsMap.find(id);
  if (val == parameterItemsMap.end()) {
    return nullptr;
  }
  return val->second;
}

const ParameterSpecItem *
ParameterRegistry::getParameterItemByParamKey(const std::string &paramKey) {
  auto val = paramKeyToIdMap.find(paramKey);
  if (val == paramKeyToIdMap.end()) {
    return nullptr;
  }
  return getParameterItemById(val->second);
}

} // namespace sonic