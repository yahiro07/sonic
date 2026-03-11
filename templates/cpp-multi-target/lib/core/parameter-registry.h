#pragma once

#include "./parameter-spec-item.h"
#include <optional>
#include <string>
#include <unordered_map>

namespace sonic {

class ParameterRegistry {
private:
  ParameterSpecArray parameterItems;
  std::unordered_map<ParamId, const ParameterSpecItem *> parameterItemsMap;
  std::unordered_map<std::string, ParamId> paramKeyToIdMap;

public:
  const ParameterSpecArray &getParameterItems() const { return parameterItems; }

  void addParameters(ParameterSpecArray &parameterItems, uint32_t maxId);

  std::optional<ParamId> getIdByParamKey(const std::string &paramKey);
  std::optional<std::string> getParamKeyById(ParamId id);
  const ParameterSpecItem *getParameterItemById(ParamId id);
  const ParameterSpecItem *
  getParameterItemByParamKey(const std::string &paramKey);
};

} // namespace sonic