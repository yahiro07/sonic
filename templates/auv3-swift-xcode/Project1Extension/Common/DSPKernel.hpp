
#pragma once

#import <AudioToolbox/AudioToolbox.h>
#import <CoreMIDI/CoreMIDI.h>
#import <algorithm>
#import <span>
#import <vector>

#include "SynthesizerBase.hpp"

/*
 DSPKernel
 As a non-ObjC class, this is safe to use from render thread.
 */
class DSPKernel {

private:
  AUHostMusicalContextBlock mMusicalContextBlock;

  double mSampleRate = 44100.0;
  bool mBypassed = false;
  AUAudioFrameCount mMaxFramesToRender = 1024;

  SynthesizerBase *mSynthInstance = nullptr;
  std::vector<AUValue> mParameterValues;
  std::vector<float> mLeftBuffer;
  std::vector<float> mRightBuffer;

public:
  void initialize(int channelCount, double inSampleRate) {
    mSampleRate = inSampleRate;

    mSynthInstance->prepareProcessing(mSampleRate, mMaxFramesToRender);
  }

  void deInitialize() {}

  void setSynthesizerInstance(SynthesizerBase *synthInstance) {
    mSynthInstance = synthInstance;
  }

  void setParametersVersion(int version) {
    if (mSynthInstance) {
      // mSynthInstance->setParametersVersion(version);
    }
  }

  // MARK: - Bypass
  bool isBypassed() { return mBypassed; }

  void setBypass(bool shouldBypass) { mBypassed = shouldBypass; }

  void setParameterCapacity(uint32_t capacity) {
    if (capacity > 4096) {
      capacity = 4096; // Arbitrary limit to prevent excessive memory usage
    }
    mParameterValues.assign(static_cast<size_t>(capacity), 0.f);
  }

  // MARK: - Parameter Getter / Setter
  // Add a case for each parameter
  void setParameter(AUParameterAddress address, AUValue value) {
    auto index = static_cast<size_t>(address);
    if (index < mParameterValues.size()) {
      std::atomic_ref<AUValue>(mParameterValues[index])
          .store(value, std::memory_order_relaxed);
    }
    if (mSynthInstance) {
      mSynthInstance->setParameter(address, value);
    }
  }

  AUValue getParameter(AUParameterAddress address) {
    auto index = static_cast<size_t>(address);
    if (index < mParameterValues.size()) {
      return std::atomic_ref<AUValue>(mParameterValues[index])
          .load(std::memory_order_relaxed);
    }
    return 0.f;
  }

  // MARK: - Max Frames
  AUAudioFrameCount maximumFramesToRender() const { return mMaxFramesToRender; }

  void setMaximumFramesToRender(const AUAudioFrameCount &maxFrames) {
    mMaxFramesToRender = maxFrames;
    mLeftBuffer.resize(maxFrames);
    mRightBuffer.resize(maxFrames);

    mSynthInstance->prepareProcessing(mSampleRate, mMaxFramesToRender);
  }

  // MARK: - Musical Context
  void setMusicalContextBlock(AUHostMusicalContextBlock contextBlock) {
    mMusicalContextBlock = contextBlock;
  }

  /**
   MARK: - Internal Process

   This function does the core siginal processing.
   Do your custom DSP here.
   */
  void process(std::span<float *> outputBuffers,
               AUEventSampleTime bufferStartTime,
               AUAudioFrameCount frameCount) {
    // Fill the 'outputBuffers' with silence
    for (UInt32 channel = 0; channel < outputBuffers.size(); ++channel) {
      std::fill_n(outputBuffers[channel], frameCount, 0.f);
    }
    if (mBypassed)
      return;
    if (!mSynthInstance)
      return;

    if (mLeftBuffer.size() == 0 || mRightBuffer.size() == 0) {
      return;
    }

    // Use this to get Musical context info from the Plugin Host,
    // Replace nullptr with &memberVariable according to the
    // AUHostMusicalContextBlock function signature
    if (mMusicalContextBlock) {
      mMusicalContextBlock(nullptr /* currentTempo */,
                           nullptr /* timeSignatureNumerator */,
                           nullptr /* timeSignatureDenominator */,
                           nullptr /* currentBeatPosition */,
                           nullptr /* sampleOffsetToNextBeat */,
                           nullptr /* currentMeasureDownbeatPosition */);
    }

    memset(mLeftBuffer.data(), 0, sizeof(float) * frameCount);
    memset(mRightBuffer.data(), 0, sizeof(float) * frameCount);

    mSynthInstance->processAudio(mLeftBuffer.data(), mRightBuffer.data(),
                                 frameCount);

    auto numChannels = outputBuffers.size();
    if (numChannels == 2) {
      auto byteLength = sizeof(float) * frameCount;
      std::memcpy(outputBuffers[0], mLeftBuffer.data(), byteLength);
      std::memcpy(outputBuffers[1], mRightBuffer.data(), byteLength);
    } else if (numChannels == 1) {
      for (UInt32 i = 0; i < frameCount; ++i) {
        auto y = outputBuffers[0][i] =
            0.5f * (mLeftBuffer[i] + mRightBuffer[i]);
      }
    }
  }

  void noteOn(int noteNumber, double velocity) {
    if (mSynthInstance) {
      mSynthInstance->noteOn(noteNumber, velocity);
    }
  }

  void noteOff(int noteNumber) {
    if (mSynthInstance) {
      mSynthInstance->noteOff(noteNumber);
    }
  }
};
