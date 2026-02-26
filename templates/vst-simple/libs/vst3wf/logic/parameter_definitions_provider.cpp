#include "./parameter_definitions_provider.h"
#include <optional>
#include <string>
#include <unordered_map>

namespace Amx {

void ParameterDefinitionsProvider::addParameters(
    std::vector<ParameterItem> &parameterItems) {
  for (const auto &item : parameterItems) {
    auto validInRange = item.address <= 0x7FFFFFFF;
    if (!validInRange) {
      printf("ParametersManager: address out of range for ParamID: %llu "
             "(%s)\n",
             static_cast<unsigned long long>(item.address),
             item.identifier.c_str());
      continue;
    }
    auto address = item.address;
    this->parameterItems[address] = item;
    this->identifierToAddressMap[item.identifier] = address;
  }
}

std::optional<ParamAddress>
ParameterDefinitionsProvider::getAddressByIdentifier(
    const std::string &identifier) {
  auto val = identifierToAddressMap.find(identifier);
  if (val == identifierToAddressMap.end()) {
    return std::nullopt;
  }
  return val->second;
}

std::string
ParameterDefinitionsProvider::getIdentifierByAddress(ParamAddress address) {
  auto val = parameterItems.find(address);
  if (val == parameterItems.end()) {
    return "";
  }
  return val->second.identifier;
}

ParameterItem *
ParameterDefinitionsProvider::getParameterItemByAddress(ParamAddress address) {
  auto val = parameterItems.find(address);
  if (val == parameterItems.end()) {
    return nullptr;
  }
  return &val->second;
}

ParameterItem *ParameterDefinitionsProvider::getParameterItemByIdentifier(
    std::string identifier) {
  auto val = identifierToAddressMap.find(identifier);
  if (val == identifierToAddressMap.end()) {
    return nullptr;
  }
  return getParameterItemByAddress(val->second);
}

} // namespace Amx