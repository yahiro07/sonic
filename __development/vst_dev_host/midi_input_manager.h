#pragma once
#include "midi_input_manager_base.h"

#include <CoreMIDI/CoreMIDI.h>
#include <vector>

class MidiInputManager : public MidiInputManagerBase {
public:
  MidiInputManager();
  ~MidiInputManager();

  std::vector<MidiDeviceInfo> enumerateDevices() override;
  void
  open(const std::string &deviceKey,
       void (*callback)(const std::vector<unsigned char> &message)) override;
  void close() override;

private:
  static void midiReadProc(const MIDIPacketList *packetList,
                           void *readProcRefCon, void *srcConnRefCon);
  void handlePacket(const MIDIPacketList *packetList);

  MIDIClientRef client_{};
  MIDIPortRef inputPort_{};
  MIDIEndpointRef connectedSource_{};
  void (*callback_)(const std::vector<unsigned char> &message){};
};