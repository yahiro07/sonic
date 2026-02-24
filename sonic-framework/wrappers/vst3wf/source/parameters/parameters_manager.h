#pragma once

#include <functional>
#include <map>
#include <string>

#include "base/source/fstring.h"
#include "parameter_builder_impl.h"
#include <public.sdk/source/vst/vsteditcontroller.h>

using namespace Steinberg;

class ParametersManager {
private:
  Vst::ParameterContainer &vstParameters;
  std::vector<ParameterItem> parameterItems;
  std::map<int, double> parametersCache;
  std::map<int, std::function<void(std::string, double)>> subscriptions;

  void addVstParameter(const ParameterItem &item) {
    auto step = 0;
    Vst::ParamValue normalizedDefaultValue = 0.;
    auto flags = 0;
    if (item.type == ParameterType::Unary) {
      step = 0;
      normalizedDefaultValue = item.defaultValue;
      flags = Vst::ParameterInfo::kCanAutomate;
    } else if (item.type == ParameterType::Enum) {
      step = item.valueStrings.size() - 1;
      normalizedDefaultValue = item.defaultValue / step;
      flags = Vst::ParameterInfo::kCanAutomate;
    } else if (item.type == ParameterType::Bool) {
      step = 1;
      normalizedDefaultValue = item.defaultValue > 0.5f ? 1 : 0;
    }

    Steinberg::String paramName;
    paramName.fromUTF8(
        reinterpret_cast<const Steinberg::char8 *>(item.label.c_str()));

    vstParameters.addParameter(
        paramName.text16(),     // name
        nullptr,                // units
        step,                   // step count (0 for continuous)
        normalizedDefaultValue, // default value (normalized)
        flags,                  // flags
        item.address            // tag (ID)
    );
  }

public:
  ParametersManager(Vst::ParameterContainer &vstParameters)
      : vstParameters(vstParameters) {}

  void addParameters(std::vector<ParameterItem> &parameterItems) {
    this->parameterItems = parameterItems;
    for (const auto &item : parameterItems) {
      auto isInternal =
          ((int)item.flags & (int)ParameterFlags::IsInternal) != 0;
      if (!isInternal) {
        addVstParameter(item);
      }
    }
  }
  void beginEdit(std::string identifier) {}
  void performEdit(std::string identifier, double value) {}
  void endEdit(std::string identifier) {}
  int subscribeFromEditor(std::function<void(std::string, double)> receiver) {
    return 0;
  }
  void unsubscribeFromEditor(int subscriptionId) {}
};