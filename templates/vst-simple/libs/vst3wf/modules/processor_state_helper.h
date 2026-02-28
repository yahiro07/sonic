#pragma once
#include <pluginterfaces/base/ibstream.h>
#include <string>
#include <unordered_map>

namespace vst3wf {

typedef struct {
  int parametersVersion;
  std::unordered_map<std::string, double> parameters;
} ProcessorState;

bool processorStateHelper_readState(Steinberg::IBStream *state,
                                    ProcessorState &processorState);

bool processorStateHelper_writeState(Steinberg::IBStream *state,
                                     const ProcessorState &processorState);

} // namespace vst3wf