#include "./parameters_manager.h"
#include "./edit_controller_parameter_change_notifier.h"
#include "./parameter_item_helper.h"
#include <base/source/fstring.h>
#include <cmath>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>

using namespace Steinberg;
using ParamAddress = Steinberg::Vst::ParamID;

void ParametersManager::addVstParameter(const ParameterItem &item) {
  auto step = ParameterItemHelper::getStepCount(&item);
  auto normDefaultValue =
      ParameterItemHelper::getNormalized(&item, item.defaultValue);
  auto flags = ParameterItemHelper::getVstFlags(&item);

  Steinberg::String paramName;
  paramName.fromUTF8(
      reinterpret_cast<const Steinberg::char8 *>(item.label.c_str()));

  vstParameters.addParameter(paramName.text16(), // name
                             nullptr,            // units
                             step,             // step count (0 for continuous)
                             normDefaultValue, // default value (normalized)
                             flags,            // flags
                             item.address      // tag (ID)
  );
}

std::optional<ParamAddress>
ParametersManager::getAddressByIdentifier(const std::string &identifier) {
  auto val = identifierToAddressMap.find(identifier);
  if (val == identifierToAddressMap.end()) {
    return std::nullopt;
  }
  return val->second;
}

std::string ParametersManager::getIdentifierByAddress(ParamAddress address) {
  auto val = parameterItems.find(address);
  if (val == parameterItems.end()) {
    return "";
  }
  return val->second.identifier;
}

ParameterItem *
ParametersManager::getParameterItemByAddress(ParamAddress address) {
  auto val = parameterItems.find(address);
  if (val == parameterItems.end()) {
    return nullptr;
  }
  return &val->second;
}

ParameterItem *
ParametersManager::getParameterItemByIdentifier(std::string identifier) {
  auto val = identifierToAddressMap.find(identifier);
  if (val == identifierToAddressMap.end()) {
    return nullptr;
  }
  return getParameterItemByAddress(val->second);
}

void ParametersManager::setEditing(ParamAddress address) {
  this->editingParamAddress = address;
}

void ParametersManager::clearEditing() {
  this->editingParamAddress = Vst::kNoParamId;
}

bool ParametersManager::checkParameterChanging(ParamAddress address,
                                               double newNormValue) {
  auto cached = parametersCache.find(address);
  if (cached == parametersCache.end()) {
    return true;
  }
  const auto oldNormValue = cached->second;
  return std::abs(oldNormValue - newNormValue) > 1e-6;
}

void ParametersManager::addParameters(
    std::vector<ParameterItem> &parameterItems) {
  for (const auto &item : parameterItems) {
    auto validInRange = item.address <= Vst::kMaxParamId; // 0x7FFFFFFF
    if (!validInRange) {
      printf("ParametersManager: address out of range for VST ParamID: %llu "
             "(%s)\n",
             static_cast<unsigned long long>(item.address),
             item.identifier.c_str());
      continue;
    }
    auto address = item.address;
    this->parameterItems[address] = item;
    this->identifierToAddressMap[item.identifier] = address;
    this->parametersCache[address] =
        ParameterItemHelper::getNormalized(&item, item.defaultValue);
    if (!item.isInternal) {
      addVstParameter(item);
    }
  }
}

// Host --> EditController --> UI
void ParametersManager::startObserve() {
  editControllerParameterChangeNotifier.start(
      &this->editController, [&](Vst::ParamID address, double normValue) {
        // logger.log("ParametersManager::notified: %d, %f", address,
        // normValue);
        if (editingParamAddress == address) {
          return;
        }
        auto paramItem = getParameterItemByAddress(address);
        if (!paramItem)
          return;

        if (!checkParameterChanging(address, normValue)) {
          return;
        }
        parametersCache[address] = normValue;

        auto value = ParameterItemHelper::getUnnormalized(paramItem, normValue);
        for (auto &receiver : uiSideReceivers) {
          receiver.second(paramItem->identifier, value);
        }
      });
}

void ParametersManager::stopObserve() {
  editControllerParameterChangeNotifier.stop();
}

// UI --> EditController --> DSP, Host
void ParametersManager::applyParameterEdit(std::string identifier, double value,
                                           ParameterEditingState editingState) {
  auto _address = getAddressByIdentifier(identifier);
  if (!_address) {
    return;
  }
  auto address = *_address;
  auto paramItem = getParameterItemByAddress(address);
  if (!paramItem || paramItem->isInternal) {
    return;
  }
  auto normValue = ParameterItemHelper::getNormalized(paramItem, value);

  if (editingState == ParameterEditingState::Perform ||
      editingState == ParameterEditingState::InstantChange) {

    if (!checkParameterChanging(address, normValue)) {
      return;
    }
    parametersCache[address] = normValue;
  }

  if (editingState == ParameterEditingState::Begin) {
    setEditing(address);
    editController.beginEdit(address);
  } else if (editingState == ParameterEditingState::Perform) {
    editController.performEdit(address, normValue);
  } else if (editingState == ParameterEditingState::End) {
    editController.endEdit(address);
    clearEditing();
  } else if (editingState == ParameterEditingState::InstantChange) {
    setEditing(address);
    editController.beginEdit(address);
    editController.performEdit(address, normValue);
    editController.endEdit(address);
    clearEditing();
  }
}

int ParametersManager::subscribeFromEditor(
    std::function<void(std::string, double)> receiver) {
  auto id = ++subscriptionIdCounter;
  uiSideReceivers[id] = receiver;
  return id;
}
void ParametersManager::unsubscribeFromEditor(int subscriptionId) {
  uiSideReceivers.erase(subscriptionId);
}

void ParametersManager::getAllParameterValues(
    std::map<std::string, double> &parameters) {
  for (auto &kv : parameterItems) {
    auto &item = kv.second;
    auto address = item.address;
    auto value = parametersCache[address];
    parameters[item.identifier] =
        ParameterItemHelper::getUnnormalized(&item, value);
  }
}
