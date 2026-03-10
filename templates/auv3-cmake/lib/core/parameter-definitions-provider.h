#pragma once

#include "./parameter-item.h"
#include <optional>
#include <string>
#include <unordered_map>

namespace sonic {

class ParameterDefinitionsProvider {
private:
  std::vector<ParameterItem> parameterItems;
  std::unordered_map<ParamId, const ParameterItem *> parameterItemsMap;
  std::unordered_map<std::string, ParamId> paramKeyToIdMap;

public:
  const std::vector<ParameterItem> &getParameterItems() const {
    return parameterItems;
  }

  void addParameters(std::vector<ParameterItem> &parameterItems,
                     uint32_t maxId);

  std::optional<ParamId> getIdByParamKey(const std::string &paramKey);
  std::optional<std::string> getParamKeyById(ParamId id);
  const ParameterItem *getParameterItemById(ParamId id);
  const ParameterItem *getParameterItemByParamKey(const std::string &paramKey);
};

} // namespace sonic