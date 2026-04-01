#pragma once
#include <pluginterfaces/base/ibstream.h>
#include <sonic/core/persistence.h>

namespace sonic_vst {

bool processorStateHelper_readState(Steinberg::IBStream *stream,
                                    sonic::PersistStateData &data);

bool processorStateHelper_writeState(Steinberg::IBStream *stream,
                                     const sonic::PersistStateData &data);

} // namespace sonic_vst