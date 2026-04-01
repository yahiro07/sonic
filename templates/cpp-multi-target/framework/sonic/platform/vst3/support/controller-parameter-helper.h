#pragma once
#include <base/source/fstring.h>
#include <pluginterfaces/base/ibstream.h>
#include <public.sdk/source/vst/vstparameters.h>
#include <sonic/core/parameter-spec-helper.h>

namespace sonic_vst {

static int getParameterVstFlags(const sonic::ParameterSpecItem *item) {
  auto flags = 0;
  flags |= Steinberg::Vst::ParameterInfo::kCanAutomate;
  if ((item->flags & sonic::ParameterFlags::IsReadOnly) >
      sonic::ParameterFlags::None) {
    flags |= Steinberg::Vst::ParameterInfo::kIsReadOnly;
  }
  if ((item->flags & sonic::ParameterFlags::IsHidden) >
      sonic::ParameterFlags::None) {
    flags |= Steinberg::Vst::ParameterInfo::kIsHidden;
  }
  return flags;
}
static void
addVstControllerParameter(Steinberg::Vst::ParameterContainer &vstParameters,
                          const sonic::ParameterSpecItem &item) {
  auto step = sonic::ParameterSpecHelper::getStepCount(&item);
  auto normDefaultValue =
      sonic::ParameterSpecHelper::getNormalized(&item, item.defaultValue);
  auto flags = getParameterVstFlags(&item);

  Steinberg::String paramName;
  paramName.fromUTF8(
      reinterpret_cast<const Steinberg::char8 *>(item.label.c_str()));

  vstParameters.addParameter(paramName.text16(), // name
                             nullptr,            // units
                             step,             // step count (0 for continuous)
                             normDefaultValue, // default value (normalized)
                             flags,            // flags
                             item.id           // tag (ID)
  );
}

} // namespace sonic_vst