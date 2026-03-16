#pragma once

#include "../../../api/synthesizer-base.h"
#include "../../../core/parameter-builder-impl.h"
#include "../../../core/parameter-registry.h"
#include "../../../core/parameter-spec-helper.h"
#include "../../../domain/parameters-store.h"

namespace sonic {

inline void initializeParameters(SynthesizerBase &synth,
                                 ParameterRegistry &parametersRegistry,
                                 ParametersStore &parametersStore) {
  auto parameterBuilder = ParameterBuilderImpl();
  synth.setupParameters(parameterBuilder);
  auto parameterItems = parameterBuilder.getItems();
  parametersRegistry.addParameters(parameterItems, 0x7FFFFFFE);
  auto maxId = ParameterSpecHelper::getMaxIdFromParameterItems(parameterItems);
  parametersStore.setup(maxId);
  for (const auto &item : parameterItems) {
    parametersStore.set(item.id, item.defaultValue);
  }
}

} // namespace sonic