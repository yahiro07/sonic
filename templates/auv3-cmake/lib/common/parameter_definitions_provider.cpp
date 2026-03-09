#include "./parameter_definitions_provider.h"
#include <optional>
#include <string>
#include <unordered_map>

namespace sonic_common {

void ParameterDefinitionsProvider::addParameters(
    std::vector<ParameterItem> &parameterItems, uint64_t maxAddress) {
  this->parameterItems.clear();
  this->parameterItemsMap.clear();
  this->identifierToAddressMap.clear();
  for (const auto &item : parameterItems) {
    auto validInRange = item.address <= maxAddress;
    if (!validInRange) {
      printf("ParametersManager: address out of range for ParamID: %llu "
             "(%s)\n",
             static_cast<unsigned long long>(item.address),
             item.identifier.c_str());
      continue;
    }
    auto address = item.address;
    this->parameterItems.push_back(item);
  }
  for (auto &item : this->parameterItems) {
    this->parameterItemsMap[item.address] = &item;
    this->identifierToAddressMap[item.identifier] = item.address;
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

std::optional<std::string>
ParameterDefinitionsProvider::getIdentifierByAddress(ParamAddress address) {
  auto val = parameterItemsMap.find(address);
  if (val == parameterItemsMap.end()) {
    return std::nullopt;
  }
  return val->second->identifier;
}

const ParameterItem *
ParameterDefinitionsProvider::getParameterItemByAddress(ParamAddress address) {
  auto val = parameterItemsMap.find(address);
  if (val == parameterItemsMap.end()) {
    return nullptr;
  }
  return val->second;
}

const ParameterItem *ParameterDefinitionsProvider::getParameterItemByIdentifier(
    std::string identifier) {
  auto val = identifierToAddressMap.find(identifier);
  if (val == identifierToAddressMap.end()) {
    return nullptr;
  }
  return getParameterItemByAddress(val->second);
}

} // namespace sonic_common