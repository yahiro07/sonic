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
    if (MIDIObjectGetStringProperty(source, kMIDIPropertyName, &name) ==
        noErr) {
      std::string displayName = copyCFString(name);
      CFRelease(name);
      devices.push_back({static_cast<int>(index), displayName});
    } else {
      devices.push_back(
          {static_cast<int>(index), std::string("Unknown MIDI Input")});
    }
  }
  return devices;
}

void MidiInputManager::open(
    int deviceId, void (*callback)(const std::vector<unsigned char> &message)) {
  close();

  if (deviceId < 0 || deviceId >= static_cast<int>(MIDIGetNumberOfSources())) {
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

  MIDIEndpointRef source = MIDIGetSource(deviceId);
  if (source == 0) {
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