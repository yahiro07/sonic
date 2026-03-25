#pragma once

#include "../../../api/synthesizer-base.h"
#include "../../../core/parameter-builder-impl.h"
#include "../../../core/parameter-registry.h"
#include "../../../core/parameter-spec-helper.h"
#include "../../../core/parameter-store.h"

namespace sonic {

inline void initializeParameters(SynthesizerBase &synth,
                                 ParameterRegistry &parametersRegistry,
                                 ParameterStore &parameterStore) {
  auto parameterBuilder = ParameterBuilderImpl();
  synth.setupParameters(parameterBuilder);
  auto parameterItems = parameterBuilder.getItems();
  parametersRegistry.addParameters(parameterItems, 0x7FFFFFFE);
  auto maxId = ParameterSpecHelper::getMaxIdFromParameterItems(parameterItems);
  parameterStore.setup(maxId);
  for (const auto &item : parameterItems) {
    parameterStore.set(item.id, item.defaultValue);
  }
}

} // namespace sonic