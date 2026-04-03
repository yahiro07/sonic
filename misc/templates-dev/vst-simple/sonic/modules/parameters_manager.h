#pragma once

#include "../logic/parameter_definitions_provider.h"
#include "../logic/parameter_item.h"
#include "./parameter_change_notifier.h"
#include <base/source/fstring.h>
#include <functional>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <string>
#include <unordered_map>

namespace sonic_vst {

enum class ParameterEditingState { Begin, Perform, End, InstantChange };

class ParametersManager {

private:
  Steinberg::Vst::EditController &editController;
  Steinberg::Vst::ParameterContainer &vstParameters;
  ParameterDefinitionsProvider &parameterDefinitionsProvider;

  ParameterChangeNotifier parameterChangeNotifier;
  // Cache is stored in normalized (0..1) space.
  std::unordered_map<ParamAddress, double> parametersCache;

  int subscriptionIdCounter = 0;
  std::unordered_map<int, std::function<void(std::string, double)>>
      uiSideReceivers;
  ParamAddress editingParamAddress = Steinberg::Vst::kNoParamId;

  void addVstParameter(const ParameterItem &item);

  void setEditing(ParamAddress address);

  void clearEditing();

  bool checkParameterChanging(ParamAddress address, double newNormValue);

public:
  ParametersManager(Steinberg::Vst::EditController &editController,
                    Steinberg::Vst::ParameterContainer &vstParameters,
                    ParameterDefinitionsProvider &parameterDefinitionsProvider)
      : editController(editController), vstParameters(vstParameters),
        parameterDefinitionsProvider(parameterDefinitionsProvider) {}

  ~ParametersManager() {}

  void addParameters(std::vector<ParameterItem> &parameterItems);

  void startObserve();

  void stopObserve();

  void applyParameterEdit(std::string identifier, double value,
                          ParameterEditingState editingState);

  int subscribeFromEditor(std::function<void(std::string, double)> receiver);

  void unsubscribeFromEditor(int subscriptionId);

  void getAllParameterValues(std::map<std::string, double> &parameters);
};

} // namespace sonic_vst