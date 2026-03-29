#pragma once
#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <vector>

class ParameterStore {
private:
  std::vector<float> mParameterValues;

public:
  ParameterStore() = default;

  void setParameterCapacity(uint32_t capacity) {
    if (capacity > 4096) {
      capacity = 4096;
    }
    mParameterValues.assign(static_cast<size_t>(capacity), 0.f);
  }

  void setParameter(AUParameterAddress address, AUValue value) {
    auto index = static_cast<size_t>(address);
    mParameterValues[index] = value;
  }

  AUValue getParameter(AUParameterAddress address) {
    auto index = static_cast<size_t>(address);
    if (index < mParameterValues.size()) {
      return mParameterValues[index];
    }
    return 0.f;
  }
};
