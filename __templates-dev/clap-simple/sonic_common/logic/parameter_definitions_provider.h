#pragma once

#include "./parameter_item.h"
#include <optional>
#include <string>
#include <unordered_map>

namespace sonic_common {

class ParameterDefinitionsProvider {
private:
  std::vector<ParameterItem> parameterItems;
  std::unordered_map<ParamAddress, const ParameterItem *> parameterItemsMap;
  std::unordered_map<std::string, ParamAddress> identifierToAddressMap;

public:
  const std::vector<ParameterItem> &getParameterItems() const {
    return parameterItems;
  }

  void addParameters(std::vector<ParameterItem> &parameterItems,
                     uint64_t maxAddress);

  std::optional<ParamAddress>
  getAddressByIdentifier(const std::string &identifier);
  std::optional<std::string> getIdentifierByAddress(ParamAddress address);
  const ParameterItem *getParameterItemByAddress(ParamAddress address);
  const ParameterItem *getParameterItemByIdentifier(std::string identifier);
};

} // namespace sonic_common