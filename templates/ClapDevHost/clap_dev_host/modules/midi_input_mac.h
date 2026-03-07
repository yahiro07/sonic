#pragma once
#include "midi_input_base.h"
#include <CoreMIDI/CoreMIDI.h>
#include <functional>
#include <vector>

namespace clap_dev_host {

class MidiInputMac : public MidiInputBase {
public:
  MidiInputMac();
  ~MidiInputMac();

  std::vector<MidiDeviceInfo> enumerateDevices() override;
  void open(const std::string &deviceKey,
            std::function<void(const unsigned char *message, size_t length)>
                callback) override;
  void close() override;

private:
  static void midiReadProc(const MIDIPacketList *packetList,
                           void *readProcRefCon, void *srcConnRefCon);
  void handlePacket(const MIDIPacketList *packetList);

  MIDIClientRef client_{};
  MIDIPortRef inputPort_{};
  MIDIEndpointRef connectedSource_{};
  std::function<void(const unsigned char *message, size_t length)> callback_ =
      nullptr;
};
} // namespace clap_dev_host