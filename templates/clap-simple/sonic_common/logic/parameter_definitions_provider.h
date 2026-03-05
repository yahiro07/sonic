#pragma once
#pragma once

#include "./parameter_item.h"
#include <optional>
#include <string>
#include <unordered_map>

namespace sonic_common {

class ParameterDefinitionsProvider {
private:
  std::unordered_map<ParamAddress, ParameterItem> parameterItems;
  std::unordered_map<std::string, ParamAddress> identifierToAddressMap;

public:
  const std::unordered_map<ParamAddress, ParameterItem> &
  getParameterItems() const {
    return parameterItems;
  }

  void addParameters(std::vector<ParameterItem> &parameterItems,
                     uint64_t maxAddress);

  std::optional<ParamAddress>
  getAddressByIdentifier(const std::string &identifier);
  std::string getIdentifierByAddress(ParamAddress address);
  ParameterItem *getParameterItemByAddress(ParamAddress address);
  ParameterItem *getParameterItemByIdentifier(std::string identifier);
};

} // namespace sonic_common