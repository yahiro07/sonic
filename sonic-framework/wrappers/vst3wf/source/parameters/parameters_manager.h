#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>

#include "base/source/fstring.h"
#include "edit_controller_parameter_change_notifier.h"
#include "parameter_builder_impl.h"
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <unordered_map>

using namespace Steinberg;

enum class ParameterEditingState { Begin, Perform, End, InstantChange };

class ParameterItemHelper {
public:
  static double getNormalized(const ParameterItem *item, double value) {
    if (item->type == ParameterType::Enum) {
      auto count = item->valueStrings.size();
      if (count < 2) {
        return 0.0;
      }
      int stepCount = count - 1;
      int idx = std::lround(value);
      int clampedIdx = std::clamp(idx, 0, stepCount);
      return static_cast<double>(clampedIdx) / static_cast<double>(stepCount);
    } else if (item->type == ParameterType::Bool) {
      return value > 0.5 ? 1.0 : 0.0;
    } else {
      return std::max(0.0, std::min(value, 1.0));
    }
  }
  static double getUnnormalized(const ParameterItem *item, double normValue) {
    if (item->type == ParameterType::Enum) {
      auto count = item->valueStrings.size();
      if (count < 2) {
        return 0.0;
      }
      auto stepCount = count - 1;
      auto clampedNorm = std::clamp(normValue, 0.0, 1.0);
      return std::lround(clampedNorm * stepCount);
    } else if (item->type == ParameterType::Bool) {
      return normValue > 0.5 ? 1.0 : 0.0;
    }
    return normValue;
  }
  static int getStepCount(const ParameterItem *item) {
    if (item->type == ParameterType::Enum) {
      auto count = item->valueStrings.size();
      if (count < 2) {
        return 0;
      }
      return count - 1;
    } else if (item->type == ParameterType::Bool) {
      return 1;
    }
    return 0;
  }
  static int getVstFlags(const ParameterItem *item) {
    auto flags = 0;
    flags |= Vst::ParameterInfo::kCanAutomate;
    if (item->flags & ParameterFlags::IsReadOnly) {
      flags |= Vst::ParameterInfo::kIsReadOnly;
    }
    if (item->flags & ParameterFlags::IsHidden) {
      flags |= Vst::ParameterInfo::kIsHidden;
    }
    return flags;
  }
};

class ParametersManager {
  typedef uint32_t ParamAddress;

private:
  Vst::EditController &editController;
  Vst::ParameterContainer &vstParameters;
  EditControllerParameterChangeNotifier editControllerParameterChangeNotifier;
  std::unordered_map<ParamAddress, ParameterItem> parameterItems;
  std::unordered_map<std::string, ParamAddress> identifierToAddressMap;
  std::unordered_map<ParamAddress, double> parametersCache; // not normalized

  int subscriptionIdCounter = 0;
  std::unordered_map<int, std::function<void(std::string, double)>>
      uiSideReceivers;

  bool isEditing = false;
  ParamAddress editingParamAddress = 0;

  void addVstParameter(const ParameterItem &item) {
    auto step = ParameterItemHelper::getStepCount(&item);
    auto normDefaultValue =
        ParameterItemHelper::getNormalized(&item, item.defaultValue);
    auto flags = ParameterItemHelper::getVstFlags(&item);

    Steinberg::String paramName;
    paramName.fromUTF8(
        reinterpret_cast<const Steinberg::char8 *>(item.label.c_str()));

    vstParameters.addParameter(paramName.text16(), // name
                               nullptr,            // units
                               step, // step count (0 for continuous)
                               normDefaultValue, // default value (normalized)
                               flags,            // flags
                               item.address      // tag (ID)
    );
  }

  std::optional<ParamAddress>
  getAddressByIdentifier(const std::string &identifier) {
    auto val = identifierToAddressMap.find(identifier);
    if (val == identifierToAddressMap.end()) {
      return std::nullopt;
    }
    return val->second;
  }

  std::string getIdentifierByAddress(ParamAddress address) {
    auto val = parameterItems.find(address);
    if (val == parameterItems.end()) {
      return "";
    }
    return val->second.identifier;
  }

  ParameterItem *getParameterItemByAddress(ParamAddress address) {
    auto val = parameterItems.find(address);
    if (val == parameterItems.end()) {
      return nullptr;
    }
    return &val->second;
  }

  ParameterItem *getParameterItemByIdentifier(std::string identifier) {
    auto val = identifierToAddressMap.find(identifier);
    if (val == identifierToAddressMap.end()) {
      return nullptr;
    }
    return getParameterItemByAddress(val->second);
  }

  void setEditing(ParamAddress address, bool isEditing) {
    this->isEditing = isEditing;
    this->editingParamAddress = address;
  }

  void clearEditing() {
    this->isEditing = false;
    this->editingParamAddress = 0;
  }

public:
  ParametersManager(Vst::EditController &editController,
                    Vst::ParameterContainer &vstParameters)
      : editController(editController), vstParameters(vstParameters) {}

  ~ParametersManager() {}

  void addParameters(std::vector<ParameterItem> &parameterItems) {
    for (const auto &item : parameterItems) {
      auto address = item.address;
      this->parameterItems[address] = item;
      this->identifierToAddressMap[item.identifier] = address;
      this->parametersCache[address] = item.defaultValue;
      if (!item.isInternal) {
        addVstParameter(item);
      }
    }
  }

  // Host --> EditController --> UI
  void startObserve() {
    editControllerParameterChangeNotifier.start(
        &this->editController, [&](ParamAddress address, double normValue) {
          if (isEditing && editingParamAddress == address) {
            return;
          }
          auto paramItem = getParameterItemByAddress(address);
          if (!paramItem)
            return;
          auto value =
              ParameterItemHelper::getUnnormalized(paramItem, normValue);
          for (auto &receiver : uiSideReceivers) {
            receiver.second(paramItem->identifier, value);
          }
        });
  }

  void stopObserve() { editControllerParameterChangeNotifier.stop(); }

  // UI --> EditController --> DSP, Host
  void applyParameterEdit(std::string identifier, double value,
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

    if (editingState == ParameterEditingState::Begin) {
      setEditing(address, true);
      editController.beginEdit(address);
    } else if (editingState == ParameterEditingState::Perform) {
      editController.performEdit(address, normValue);
    } else if (editingState == ParameterEditingState::End) {
      editController.endEdit(address);
      clearEditing();
    } else if (editingState == ParameterEditingState::InstantChange) {
      setEditing(address, true);
      editController.beginEdit(address);
      editController.performEdit(address, normValue);
      editController.endEdit(address);
      clearEditing();
    }
  }

  int subscribeFromEditor(std::function<void(std::string, double)> receiver) {
    auto id = ++subscriptionIdCounter;
    uiSideReceivers[id] = receiver;
    return id;
  }
  void unsubscribeFromEditor(int subscriptionId) {
    uiSideReceivers.erase(subscriptionId);
  }
};