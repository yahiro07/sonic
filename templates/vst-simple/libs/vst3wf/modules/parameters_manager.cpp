#include "./parameters_manager.h"
#include "../logic/parameter_item_helper.h"
#include <base/source/fstring.h>
#include <cmath>
#include <functional>
#include <string>
#include <unordered_map>

namespace vst3wf {

using namespace Steinberg;

static int getParameterVstFlags(const ParameterItem *item) {
  auto flags = 0;
  flags |= Steinberg::Vst::ParameterInfo::kCanAutomate;
  if (item->flags & ParameterFlags::IsReadOnly) {
    flags |= Steinberg::Vst::ParameterInfo::kIsReadOnly;
  }
  if (item->flags & ParameterFlags::IsHidden) {
    flags |= Steinberg::Vst::ParameterInfo::kIsHidden;
  }
  return flags;
}

void ParametersManager::addVstParameter(const ParameterItem &item) {
  auto step = ParameterItemHelper::getStepCount(&item);
  auto normDefaultValue =
      ParameterItemHelper::getNormalized(&item, item.defaultValue);
  auto flags = getParameterVstFlags(&item);

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
  parameterDefinitionsProvider.addParameters(parameterItems);
  for (const auto &item : parameterItems) {
    addVstParameter(item);
    this->parametersCache[item.address] =
        ParameterItemHelper::getNormalized(&item, item.defaultValue);
  }
}

// Host --> EditController --> UI
void ParametersManager::startObserve() {
  parameterChangeNotifier.start(
      &this->editController, [&](Vst::ParamID address, double normValue) {
        // logger.log("ParametersManager::notified: %d, %f", address,
        // normValue);
        if (editingParamAddress == address) {
          return;
        }
        auto paramItem =
            parameterDefinitionsProvider.getParameterItemByAddress(address);
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

void ParametersManager::stopObserve() { parameterChangeNotifier.stop(); }

// UI --> EditController --> DSP, Host
void ParametersManager::applyParameterEdit(std::string identifier, double value,
                                           ParameterEditingState editingState) {
  auto _address =
      parameterDefinitionsProvider.getAddressByIdentifier(identifier);
  if (!_address) {
    return;
  }
  auto address = *_address;
  auto paramItem =
      parameterDefinitionsProvider.getParameterItemByAddress(address);
  if (!paramItem) {
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
  const auto &parameterItems = parameterDefinitionsProvider.getParameterItems();
  for (auto &kv : parameterItems) {
    auto &item = kv.second;
    auto address = item.address;
    auto value = parametersCache[address];
    parameters[item.identifier] =
        ParameterItemHelper::getUnnormalized(&item, value);
  }
}

} // namespace vst3wf