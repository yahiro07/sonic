module;
#include "../api/synthesizer-base.h"
#include <algorithm>
#include <cmath>

export module core:parameter_spec_helper;
import :parameter_spec_item;

namespace sonic {

export class ParameterSpecHelper {
public:
  static double getNormalized(const ParameterSpecItem *item, double value) {
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
  static double getUnnormalized(const ParameterSpecItem *item,
                                double normValue) {
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

  static int getMaxIdFromParameterItems(const ParameterSpecArray &items) {
    int maxId = 0;
    for (const auto &item : items) {
      if (item.id > maxId) {
        maxId = item.id;
      }
    }
    return maxId;
  }
};

} // namespace sonic