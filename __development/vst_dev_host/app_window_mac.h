#pragma once

#include "app_window_base.h"
#include <memory>

class AppWindowMac : public AppWindowBase {
public:
  AppWindowMac();
  ~AppWindowMac();

  void show() override;
  void loop() override;
  void
  refreshMidiInputDeviceListMenu(const std::vector<MidiDeviceInfo> &devices,
                                 const std::string &selectedDeviceKey) override;
  void subscribeMidiInputDeviceSelection(
      void (*callback)(const std::string &deviceKey)) override;
  void clearMenuSubscriptions() override;

private:
  struct InternalStates;
  std::unique_ptr<InternalStates> states;
};