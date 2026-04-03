#pragma once

#include "./parameter_item.h"
#include <algorithm>
#include <cmath>

namespace sonic_common {

class ParameterItemHelper {
public:
  static double getNormalized(const ParameterItem *item, double value) {
    if (item->type == ParameterType::Enum) {
      auto count = item->valueStrings.size();
      if (count < 2) {
        return 0.0;
      }
      int stepCount = count - 1;
      int idx = std::lround(value);
      int clampedIdx = std::clamp(idx, 0, stepCount);
      return static_cast<double>(clampedIdx) / static_cast<double>(stepCount);
    } else if (item->type == ParameterType::Bool) {
      return value > 0.5 ? 1.0 : 0.0;
    } else {
      return std::max(0.0, std::min(value, 1.0));
    }
  }
  static double getUnnormalized(const ParameterItem *item, double normValue) {
    if (item->type == ParameterType::Enum) {
      auto count = item->valueStrings.size();
      if (count < 2) {
        return 0.0;
      }
      auto stepCount = count - 1;
      auto clampedNorm = std::clamp(normValue, 0.0, 1.0);
      return std::lround(clampedNorm * stepCount);
    } else if (item->type == ParameterType::Bool) {
      return normValue > 0.5 ? 1.0 : 0.0;
    }
    return normValue;
  }
  static int getStepCount(const ParameterItem *item) {
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

} // namespace sonic_common