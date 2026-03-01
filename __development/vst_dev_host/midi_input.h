#pragma once

#include <CoreMIDI/CoreMIDI.h>
#include <string>
#include <vector>

typedef struct {
  int deviceId;
  std::string name;
} MidiDeviceInfo;

class MidiInputManager {
public:
  MidiInputManager();
  ~MidiInputManager();

  static std::vector<MidiDeviceInfo> enumerateDevices();
  void open(int deviceId,
            void (*callback)(const std::vector<unsigned char> &message));
  void close();

private:
  static void midiReadProc(const MIDIPacketList *packetList,
                           void *readProcRefCon, void *srcConnRefCon);
  void handlePacket(const MIDIPacketList *packetList);

  MIDIClientRef client_{};
  MIDIPortRef inputPort_{};
  MIDIEndpointRef connectedSource_{};
  void (*callback_)(const std::vector<unsigned char> &message){};
};