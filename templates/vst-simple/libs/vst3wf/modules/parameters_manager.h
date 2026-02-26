#pragma once

#include "../logic/parameter_definitions_provider.h"
#include "../logic/parameter_item.h"
#include "./edit_controller_parameter_change_notifier.h"
#include <base/source/fstring.h>
#include <functional>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <string>
#include <unordered_map>

namespace vst3wf {

enum class ParameterEditingState { Begin, Perform, End, InstantChange };

class ParametersManager {

private:
  Steinberg::Vst::EditController &editController;
  Steinberg::Vst::ParameterContainer &vstParameters;
  ParameterDefinitionsProvider parameterDefinitionsProvider;

  EditControllerParameterChangeNotifier editControllerParameterChangeNotifier;
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
                    Steinberg::Vst::ParameterContainer &vstParameters)
      : editController(editController), vstParameters(vstParameters) {}

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

} // namespace vst3wf