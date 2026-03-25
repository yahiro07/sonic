#pragma once

#include <sonic/api/synthesizer-base.h>
#include <sonic/core/parameter-builder-impl.h>
#include <sonic/core/parameter-registry.h>
#include <sonic/core/parameter-spec-helper.h>
#include <sonic/core/parameter-store.h>

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