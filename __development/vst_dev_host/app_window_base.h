#include "midi_input_manager_base.h"

class AppWindowBase {
public:
  virtual ~AppWindowBase() = default;
  virtual void show() = 0;
  virtual void loop() = 0;
  virtual void
  refreshMidiInputDeviceListMenu(const std::vector<MidiDeviceInfo> &devices,
                                 const std::string &selectedDeviceKey) = 0;
  virtual void subscribeMidiInputDeviceSelection(
      void (*callback)(const std::string &deviceKey)) = 0;
  virtual void clearMenuSubscriptions() = 0;
};
