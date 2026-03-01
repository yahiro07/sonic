#include "./audio_io_base.h"
#include "./midi_input_base.h"
#include <functional>

namespace vst_dev_host {

class AppWindowBase {
public:
  virtual ~AppWindowBase() = default;
  virtual void show() = 0;
  virtual void loop() = 0;
  virtual void
  refreshMidiInputDeviceListMenu(const std::vector<MidiDeviceInfo> &devices,
                                 const std::string &selectedDeviceKey) = 0;
  virtual void subscribeMidiInputDeviceSelection(
      std::function<void(const std::string &deviceKey)> callback) = 0;
  virtual void unsubscribeMidiInputDeviceSelection() = 0;

  virtual void
  refreshAudioDeviceListMenu(const std::vector<AudioDeviceInfo> &devices,
                             const std::string &selectedDeviceKey) = 0;
  virtual void subscribeAudioDeviceSelection(
      std::function<void(const std::string &deviceKey)> callback) = 0;
  virtual void unsubscribeAudioDeviceSelection() = 0;
};

} // namespace vst_dev_host