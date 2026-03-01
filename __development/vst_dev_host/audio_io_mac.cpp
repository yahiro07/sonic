#include "audio_io_mac.h"
#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>
#include <algorithm>
#include <cstdio>

AudioIoMac::AudioIoMac() = default;

AudioIoMac::~AudioIoMac() { close(); }

std::vector<AudioDeviceInfo> AudioIoMac::enumerateDevices() {
  std::vector<AudioDeviceInfo> devices;
  devices.push_back({"DefaultAudioDevice", "Default Audio Device"});

  AudioObjectPropertyAddress propertyAddress = {
      kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal,
      kAudioObjectPropertyElementMain};

  UInt32 dataSize = 0;
  OSStatus status = AudioObjectGetPropertyDataSize(
      kAudioObjectSystemObject, &propertyAddress, 0, nullptr, &dataSize);
  if (status != noErr)
    return devices;

  UInt32 deviceCount = dataSize / sizeof(AudioDeviceID);
  std::vector<AudioDeviceID> audioDevices(deviceCount);
  status =
      AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0,
                                 nullptr, &dataSize, audioDevices.data());
  if (status != noErr)
    return devices;

  for (UInt32 i = 0; i < deviceCount; ++i) {
    AudioDeviceID deviceID = audioDevices[i];

    // Get device name
    CFStringRef deviceName = nullptr;
    dataSize = sizeof(CFStringRef);
    propertyAddress.mSelector = kAudioObjectPropertyName;
    status = AudioObjectGetPropertyData(deviceID, &propertyAddress, 0, nullptr,
                                        &dataSize, &deviceName);
    if (status != noErr)
      continue;

    char nameBuf[256];
    CFStringGetCString(deviceName, nameBuf, sizeof(nameBuf),
                       kCFStringEncodingUTF8);
    CFRelease(deviceName);

    // Get UID for deviceKey
    CFStringRef deviceUID = nullptr;
    dataSize = sizeof(CFStringRef);
    propertyAddress.mSelector = kAudioDevicePropertyDeviceUID;
    status = AudioObjectGetPropertyData(deviceID, &propertyAddress, 0, nullptr,
                                        &dataSize, &deviceUID);
    if (status != noErr)
      continue;

    char uidBuf[256];
    CFStringGetCString(deviceUID, uidBuf, sizeof(uidBuf),
                       kCFStringEncodingUTF8);
    CFRelease(deviceUID);

    devices.push_back({uidBuf, nameBuf});
  }

  return devices;
}

void AudioIoMac::open(
    const std::string &deviceKey, bool enableInput,
    std::function<void(double sampleRate, int maxFrameLength)> prepareFn,
    std::function<void(float *bufferL, float *bufferR, int nframes)>
        processFn) {
  close();

  this->prepareFn = prepareFn;
  this->processFn = processFn;

  AudioDeviceID targetDevice = kAudioObjectUnknown;
  if (deviceKey == "DefaultAudioDevice") {
    AudioObjectPropertyAddress propertyAddress = {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
    UInt32 dataSize = sizeof(AudioDeviceID);
    AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0,
                               nullptr, &dataSize, &targetDevice);
  } else {
    // Find device by UID
    AudioObjectPropertyAddress propertyAddress = {
        kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain};
    UInt32 dataSize = 0;
    AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &propertyAddress,
                                   0, nullptr, &dataSize);
    UInt32 deviceCount = dataSize / sizeof(AudioDeviceID);
    std::vector<AudioDeviceID> audioDevices(deviceCount);
    AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0,
                               nullptr, &dataSize, audioDevices.data());

    for (auto devID : audioDevices) {
      CFStringRef deviceUID = nullptr;
      dataSize = sizeof(CFStringRef);
      propertyAddress.mSelector = kAudioDevicePropertyDeviceUID;
      if (AudioObjectGetPropertyData(devID, &propertyAddress, 0, nullptr,
                                     &dataSize, &deviceUID) == noErr) {
        char uidBuf[256];
        CFStringGetCString(deviceUID, uidBuf, sizeof(uidBuf),
                           kCFStringEncodingUTF8);
        CFRelease(deviceUID);
        if (deviceKey == uidBuf) {
          targetDevice = devID;
          break;
        }
      }
    }
  }

  if (targetDevice == kAudioObjectUnknown)
    return;

  AudioComponentDescription desc;
  desc.componentType = kAudioUnitType_Output;
  desc.componentSubType = kAudioUnitSubType_HALOutput;
  desc.componentManufacturer = kAudioUnitManufacturer_Apple;
  desc.componentFlags = 0;
  desc.componentFlagsMask = 0;

  AudioComponent comp = AudioComponentFindNext(nullptr, &desc);
  if (!comp)
    return;

  if (AudioComponentInstanceNew(comp, &audioUnit) != noErr)
    return;

  // Set individual device
  if (AudioUnitSetProperty(audioUnit, kAudioOutputUnitProperty_CurrentDevice,
                           kAudioUnitScope_Global, 0, &targetDevice,
                           sizeof(AudioDeviceID)) != noErr) {
    return;
  }

  // Enable Input if requested
  UInt32 enableIO = enableInput ? 1 : 0;
  AudioUnitSetProperty(audioUnit, kAudioOutputUnitProperty_EnableIO,
                       kAudioUnitScope_Input, 1, &enableIO, sizeof(enableIO));

  // Enable Output (always)
  enableIO = 1;
  AudioUnitSetProperty(audioUnit, kAudioOutputUnitProperty_EnableIO,
                       kAudioUnitScope_Output, 0, &enableIO, sizeof(enableIO));

  // Get device format and setup stream formats
  AudioStreamBasicDescription format;
  UInt32 size = sizeof(format);
  AudioUnitGetProperty(audioUnit, kAudioUnitProperty_StreamFormat,
                       kAudioUnitScope_Output, 0, &format, &size);

  double sampleRate = format.mSampleRate;

  // Set internal formats to Float32 Non-Interleaved
  format.mSampleRate = sampleRate;
  format.mFormatID = kAudioFormatLinearPCM;
  format.mFormatFlags =
      kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved;
  format.mBytesPerPacket = 4;
  format.mFramesPerPacket = 1;
  format.mBytesPerFrame = 4;
  format.mChannelsPerFrame = 2; // We want stereo processing
  format.mBitsPerChannel = 32;

  AudioUnitSetProperty(audioUnit, kAudioUnitProperty_StreamFormat,
                       kAudioUnitScope_Input, 0, &format, sizeof(format));

  AURenderCallbackStruct callbackStruct;
  callbackStruct.inputProc = inputProc;
  callbackStruct.inputProcRefCon = this;
  AudioUnitSetProperty(audioUnit, kAudioUnitProperty_SetRenderCallback,
                       kAudioUnitScope_Input, 0, &callbackStruct,
                       sizeof(callbackStruct));

  if (AudioUnitInitialize(audioUnit) != noErr)
    return;

  // Get max frames
  UInt32 maxFrames = 0;
  size = sizeof(maxFrames);
  AudioUnitGetProperty(audioUnit, kAudioUnitProperty_MaximumFramesPerSlice,
                       kAudioUnitScope_Global, 0, &maxFrames, &size);

  bufferL.assign(maxFrames, 0.0f);
  bufferR.assign(maxFrames, 0.0f);

  if (this->prepareFn) {
    this->prepareFn(sampleRate, (int)maxFrames);
  }
  if (AudioOutputUnitStart(audioUnit) == noErr) {
    isRunning = true;
  }
}

void AudioIoMac::close() {
  if (audioUnit) {
    AudioOutputUnitStop(audioUnit);
    AudioUnitUninitialize(audioUnit);
    AudioComponentInstanceDispose(audioUnit);
    audioUnit = nullptr;
  }
  isRunning = false;
}

OSStatus AudioIoMac::inputProc(void *inRefCon,
                               AudioUnitRenderActionFlags *ioActionFlags,
                               const AudioTimeStamp *inTimeStamp,
                               UInt32 inBusNumber, UInt32 inNumberFrames,
                               AudioBufferList *ioData) {
  AudioIoMac *self = (AudioIoMac *)inRefCon;

  if (self->processFn) {
    if (ioData->mNumberBuffers >= 2 && ioData->mBuffers[0].mData &&
        ioData->mBuffers[1].mData) {
      // Fast path: Zero-copy directly into hardware buffers
      self->processFn((float *)ioData->mBuffers[0].mData,
                      (float *)ioData->mBuffers[1].mData, (int)inNumberFrames);
    } else if (ioData->mNumberBuffers == 1 && ioData->mBuffers[0].mData) {
      // Mono hardware fallback: use internal buffers and mix down
      // Zero internal buffers first, then call processFn.
      std::fill(self->bufferL.begin(), self->bufferL.begin() + inNumberFrames,
                0.0f);
      std::fill(self->bufferR.begin(), self->bufferR.begin() + inNumberFrames,
                0.0f);

      self->processFn(self->bufferL.data(), self->bufferR.data(),
                      (int)inNumberFrames);
      float *out = (float *)ioData->mBuffers[0].mData;
      for (UInt32 i = 0; i < inNumberFrames; ++i) {
        out[i] = (self->bufferL[i] + self->bufferR[i]) * 0.5f;
      }
    }
  }

  return noErr;
}
