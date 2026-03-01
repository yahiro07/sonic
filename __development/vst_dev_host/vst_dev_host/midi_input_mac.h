#pragma once
#include "midi_input_base.h"
#include <CoreMIDI/CoreMIDI.h>
#include <functional>
#include <vector>

class MidiInputMac : public MidiInputBase {
public:
  MidiInputMac();
  ~MidiInputMac();

  std::vector<MidiDeviceInfo> enumerateDevices() override;
  void open(const std::string &deviceKey,
            std::function<void(const std::vector<unsigned char> &message)>
                callback) override;
  void close() override;

private:
  static void midiReadProc(const MIDIPacketList *packetList,
                           void *readProcRefCon, void *srcConnRefCon);
  void handlePacket(const MIDIPacketList *packetList);

  MIDIClientRef client_{};
  MIDIPortRef inputPort_{};
  MIDIEndpointRef connectedSource_{};
  std::function<void(const std::vector<unsigned char> &message)> callback_ =
      nullptr;
};