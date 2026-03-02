#pragma once
#include "./app_window_base.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace vst_dev_host {

class AppWindowMac : public AppWindowBase {
public:
  AppWindowMac();
  ~AppWindowMac() override;

  void show() override;
  void loop() override;
  void *getViewHandle() override;

  void
  refreshMidiInputDeviceListMenu(const std::vector<MidiDeviceInfo> &devices,
                                 const std::string &selectedDeviceKey) override;
  void subscribeMidiInputDeviceSelection(
      std::function<void(const std::string &deviceKey)> callback) override;
  void unsubscribeMidiInputDeviceSelection() override;

  void
  refreshAudioDeviceListMenu(const std::vector<AudioDeviceInfo> &devices,
                             const std::string &selectedDeviceKey) override;
  void subscribeAudioDeviceSelection(
      std::function<void(const std::string &deviceKey)> callback) override;
  void unsubscribeAudioDeviceSelection() override;

private:
  struct InternalStates;
  std::unique_ptr<InternalStates> states;
};

} // namespace vst_dev_host