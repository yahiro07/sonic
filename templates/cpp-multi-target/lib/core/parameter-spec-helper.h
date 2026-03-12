#pragma once

#include "./parameter-spec-item.h"
#include <algorithm>
#include <cmath>

namespace sonic {

class ParameterSpecHelper {
public:
  static float getNormalized(const ParameterSpecItem *item, float value) {
    if (item->type == ParameterType::Enum) {
      auto count = item->valueStrings.size();
      if (count < 2) {
        return 0.f;
      }
      int stepCount = count - 1;
      int idx = std::lround(value);
      int clampedIdx = std::clamp(idx, 0, stepCount);
      return static_cast<float>(clampedIdx) / static_cast<float>(stepCount);
    } else if (item->type == ParameterType::Bool) {
      return value > 0.5f ? 1.0f : 0.0f;
    } else {
      return std::max(0.0f, std::min(value, 1.0f));
    }
  }
  static float getUnnormalized(const ParameterSpecItem *item, float normValue) {
    if (item->type == ParameterType::Enum) {
      auto count = item->valueStrings.size();
      if (count < 2) {
        return 0.0f;
      }
      auto stepCount = count - 1;
      auto clampedNorm = std::clamp(normValue, 0.f, 1.f);
      return std::lround(clampedNorm * stepCount);
    } else if (item->type == ParameterType::Bool) {
      return normValue > 0.5f ? 1.0f : 0.0f;
    }
    return normValue;
  }
  static int getStepCount(const ParameterSpecItem *item) {
    if (item->type == ParameterType::Enum) {
      auto count = item->valueStrings.size();
      if (count < 2) {
        return 0;
      }
      return count - 1;
    } else if (item->type == ParameterType::Bool) {
      return 1;
    }
    return 0;
  }
};

} // namespace sonic