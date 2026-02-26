#pragma once
#pragma once

#include "../logic/parameter_item.h"
#include <optional>
#include <string>
#include <unordered_map>

namespace Amx {

class ParameterDefinitionsProvider {
private:
  std::unordered_map<ParamAddress, ParameterItem> parameterItems;
  std::unordered_map<std::string, ParamAddress> identifierToAddressMap;

public:
  std::unordered_map<ParamAddress, ParameterItem> &getParameterItems() {
    return parameterItems;
  }

  void addParameters(std::vector<ParameterItem> &parameterItems);

  std::optional<ParamAddress>
  getAddressByIdentifier(const std::string &identifier);
  std::string getIdentifierByAddress(ParamAddress address);
  ParameterItem *getParameterItemByAddress(ParamAddress address);
  ParameterItem *getParameterItemByIdentifier(std::string identifier);
};

} // namespace Amx