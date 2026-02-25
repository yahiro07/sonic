#pragma once

#include "./edit_controller_parameter_change_notifier.h"
#include "vst3wf/logic/parameter_item.h"
#include <base/source/fstring.h>
#include <functional>
#include <optional>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <string>
#include <unordered_map>

enum class ParameterEditingState { Begin, Perform, End, InstantChange };

class ParametersManager {
  using ParamAddress = Steinberg::Vst::ParamID;

private:
  Steinberg::Vst::EditController &editController;
  Steinberg::Vst::ParameterContainer &vstParameters;
  EditControllerParameterChangeNotifier editControllerParameterChangeNotifier;
  std::unordered_map<ParamAddress, ParameterItem> parameterItems;
  std::unordered_map<std::string, ParamAddress> identifierToAddressMap;
  // Cache is stored in normalized (0..1) space.
  std::unordered_map<ParamAddress, double> parametersCache;

  int subscriptionIdCounter = 0;
  std::unordered_map<int, std::function<void(std::string, double)>>
      uiSideReceivers;
  ParamAddress editingParamAddress = Steinberg::Vst::kNoParamId;

  void addVstParameter(const ParameterItem &item);

  std::optional<ParamAddress>
  getAddressByIdentifier(const std::string &identifier);

  std::string getIdentifierByAddress(ParamAddress address);

  ParameterItem *getParameterItemByAddress(ParamAddress address);

  ParameterItem *getParameterItemByIdentifier(std::string identifier);

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