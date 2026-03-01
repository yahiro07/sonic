#include "midi_input_manager.h"
#include <CoreFoundation/CoreFoundation.h>
#include <CoreMIDI/CoreMIDI.h>
#include <cstring>
#include <vector>

static std::string copyCFString(CFStringRef source) {
  if (!source) {
    return std::string();
  }

  const char *cString = CFStringGetCStringPtr(source, kCFStringEncodingUTF8);
  if (cString) {
    return std::string(cString);
  }

  CFIndex length = CFStringGetLength(source);
  CFIndex maxSize =
      CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
  std::vector<char> buffer(static_cast<size_t>(maxSize));
  if (CFStringGetCString(source, buffer.data(), maxSize,
                         kCFStringEncodingUTF8)) {
    return std::string(buffer.data());
  }

  return std::string();
}

MidiInputManager::MidiInputManager() = default;

MidiInputManager::~MidiInputManager() { close(); }

std::vector<MidiDeviceInfo> MidiInputManager::enumerateDevices() {
  std::vector<MidiDeviceInfo> devices;
  ItemCount sourceCount = MIDIGetNumberOfSources();
  for (ItemCount index = 0; index < sourceCount; ++index) {
    MIDIEndpointRef source = MIDIGetSource(index);
    CFStringRef name = nullptr;
    std::string displayName = "Unknown MIDI Input";
    if (MIDIObjectGetStringProperty(source, kMIDIPropertyName, &name) ==
        noErr) {
      displayName = copyCFString(name);
      CFRelease(name);
    }

    std::string deviceKey = "0";
    SInt32 uniqueID = 0;
    if (MIDIObjectGetIntegerProperty(source, kMIDIPropertyUniqueID,
                                     &uniqueID) == noErr) {
      deviceKey = std::to_string(uniqueID);
    }

    devices.push_back({deviceKey, displayName});
  }
  return devices;
}

void MidiInputManager::open(
    const std::string &deviceKey,
    void (*callback)(const std::vector<unsigned char> &message)) {
  close();

  MIDIEndpointRef source = 0;
  ItemCount sourceCount = MIDIGetNumberOfSources();
  for (ItemCount i = 0; i < sourceCount; ++i) {
    MIDIEndpointRef src = MIDIGetSource(i);
    SInt32 uniqueID = 0;
    if (MIDIObjectGetIntegerProperty(src, kMIDIPropertyUniqueID, &uniqueID) ==
        noErr) {
      if (std::to_string(uniqueID) == deviceKey) {
        source = src;
        break;
      }
    }
  }

  if (source == 0) {
    return;
  }

  callback_ = callback;
  OSStatus status =
      MIDIClientCreate(CFSTR("VST Midi Client"), nullptr, nullptr, &client_);
  if (status != noErr) {
    callback_ = nullptr;
    return;
  }

  status = MIDIInputPortCreate(client_, CFSTR("VST Midi Input"), midiReadProc,
                               this, &inputPort_);
  if (status != noErr) {
    close();
    return;
  }

  status = MIDIPortConnectSource(inputPort_, source, nullptr);
  if (status != noErr) {
    close();
    return;
  }

  connectedSource_ = source;
}

void MidiInputManager::close() {
  if (inputPort_) {
    if (connectedSource_) {
      MIDIPortDisconnectSource(inputPort_, connectedSource_);
      connectedSource_ = 0;
    }
    MIDIPortDispose(inputPort_);
    inputPort_ = 0;
  }

  if (client_) {
    MIDIClientDispose(client_);
    client_ = 0;
  }

  callback_ = nullptr;
}

void MidiInputManager::midiReadProc(const MIDIPacketList *packetList,
                                    void *readProcRefCon,
                                    void * /* srcConnRefCon */) {
  auto *self = static_cast<MidiInputManager *>(readProcRefCon);
  if (self) {
    self->handlePacket(packetList);
  }
}

void MidiInputManager::handlePacket(const MIDIPacketList *packetList) {
  if (!packetList || !callback_) {
    return;
  }

  const MIDIPacket *packet = &packetList->packet[0];
  for (UInt32 packetIndex = 0; packetIndex < packetList->numPackets;
       ++packetIndex) {
    std::vector<unsigned char> message(packet->length);
    if (packet->length > 0) {
      std::memcpy(message.data(), packet->data, packet->length);
    }
    callback_(message);
    packet = MIDIPacketNext(packet);
  }
}