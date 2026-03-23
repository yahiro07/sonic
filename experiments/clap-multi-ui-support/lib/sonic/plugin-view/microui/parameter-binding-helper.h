#pragma once

#include <cmath>
#include <functional>
#include <optional>
#include <sonic/core/editor-interfaces.h>

namespace sonic_plugin_view_microui {

struct BindingItem {
  uint32_t paramId;
  std::function<float()> getUiValue;
  float cachedValue;
};

class ParameterBindingHelper {
private:
  sonic::IControllerFacade &controllerFacade;

  std::map<std::string, float> initialParameters;

  std::vector<BindingItem> bindingItems;

  void bindParamFn(std::string paramKey, std::function<float()> getUiValue) {
    auto _paramId = controllerFacade.getParameterIdByParamKey(paramKey);
    if (_paramId == std::nullopt)
      return;
    auto paramId = *_paramId;

    BindingItem bindingItem{
        .paramId = paramId,
        .getUiValue = getUiValue,
        .cachedValue = getUiValue(),
    };

    bindingItems.push_back(bindingItem);
  }

public:
  explicit ParameterBindingHelper(sonic::IControllerFacade &controllerFacade)
      : controllerFacade(controllerFacade) {
    auto paramDefs = controllerFacade.getParameterSpecs();
    for (const auto &paramDef : paramDefs) {
      initialParameters[paramDef.paramKey] = paramDef.defaultValue;
    }
  }

  void bindFloat(float &value, std::string paramKey) {
    value = initialParameters[paramKey];
    bindParamFn(paramKey, [&value]() { return value; });
  }

  void bindInt(int &value, std::string paramKey) {
    value = (int)roundf(initialParameters[paramKey]);
    bindParamFn(paramKey, [&value]() { return (float)value; });
  }

  void sync() {
    for (auto &bindingItem : bindingItems) {
      auto newValue = bindingItem.getUiValue();
      if (newValue != bindingItem.cachedValue) {
        controllerFacade.applyParameterEditFromUi(
            bindingItem.paramId, newValue, sonic::ParameterEditState::Perform);
        bindingItem.cachedValue = newValue;
      }
    }
  }
};
} // namespace sonic_plugin_view_microui