#pragma once

#include "clap/ext/params.h"
#include "my_clap_1/portable/events.h"
#include "sonic_common/logic/parameter_item.h"
#include "sonic_common/synthesizer_base.h"

static clap_param_info_flags mapParameterFlags(ParameterFlags flags) {
  clap_param_info_flags clapFlags = 0;
  if (flags & ParameterFlags::IsReadOnly) {
    clapFlags |= CLAP_PARAM_IS_READONLY;
  }
  if (flags & ParameterFlags::IsHidden) {
    clapFlags |= CLAP_PARAM_IS_HIDDEN;
  }
  if (!(flags & ParameterFlags::NonAutomatable)) {
    clapFlags |= CLAP_PARAM_IS_AUTOMATABLE;
  }
  return clapFlags;
}

static void assignParameterInfo(clap_param_info_t *info,
                                const sonic_common::ParameterItem &item) {
  info->id = item.address;
  info->flags = mapParameterFlags(item.flags);
  info->min_value = item.minValue;
  info->max_value = item.maxValue;
  info->default_value = item.defaultValue;
  snprintf(info->name, sizeof(info->name), "%s", item.label.c_str());
}

static void
mapUpstreamParamChangeEventToClapEvent(UpstreamEvent &upstreamEvent,
                                       clap_event_param_value_t &clapEvent) {
  clapEvent.header.size = sizeof(clapEvent);
  clapEvent.header.time = 0;
  clapEvent.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
  clapEvent.header.type = CLAP_EVENT_PARAM_VALUE;
  clapEvent.header.flags = 0;
  clapEvent.param_id = upstreamEvent.param.paramId;
  clapEvent.cookie = NULL;
  clapEvent.note_id = -1;
  clapEvent.port_index = -1;
  clapEvent.channel = -1;
  clapEvent.key = -1;
  clapEvent.value = upstreamEvent.param.value;
}

static void
mapUpstreamParamGestureEventToClapEvent(UpstreamEvent &upstreamEvent,
                                        clap_event_param_gesture_t &clapEvent) {
  clapEvent.header.size = sizeof(clapEvent);
  clapEvent.header.time = 0;
  clapEvent.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
  clapEvent.header.type =
      upstreamEvent.type == UpstreamEventType::ParameterBeginEdit
          ? CLAP_EVENT_PARAM_GESTURE_BEGIN
          : CLAP_EVENT_PARAM_GESTURE_END;
  clapEvent.header.flags = 0;
  clapEvent.param_id = upstreamEvent.param.paramId;
}