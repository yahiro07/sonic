#pragma once
#include <cstdint>
#include <sonic/api/synthesizer-base.h>

namespace sonic {

using ParamId = uint32_t;
using IPluginSynthesizer = SynthesizerBase;

inline ParameterFlags operator|(ParameterFlags a, ParameterFlags b) {
  return static_cast<ParameterFlags>(static_cast<int>(a) | static_cast<int>(b));
}
inline ParameterFlags operator&(ParameterFlags a, ParameterFlags b) {
  return static_cast<ParameterFlags>(static_cast<int>(a) & static_cast<int>(b));
}

} // namespace sonic