#pragma once
#include "./clap-data-helper.h"
#include "./interfaces.h"
#include <cassert>

namespace sonic {

class ProcessorAdapter {
private:
  IPluginSynthesizer &synth;
  IEventBridge &eventBridge;

  void pushDownstreamEvent(DownstreamEvent e) {
    eventBridge.pushDownstreamEvent(e);
  }

  void processInputEvent(const clap_event_header_t *event) {
    if (event->space_id == CLAP_CORE_EVENT_SPACE_ID) {
      if (event->type == CLAP_EVENT_NOTE_ON ||
          event->type == CLAP_EVENT_NOTE_OFF) {
        const clap_event_note_t *noteEvent = (const clap_event_note_t *)event;
        if (event->type == CLAP_EVENT_NOTE_ON) {
          synth.noteOn(noteEvent->key, noteEvent->velocity);
          pushDownstreamEvent({.type = DownstreamEventType::HostNote,
                               .note = {.noteNumber = noteEvent->key,
                                        .velocity = noteEvent->velocity}});
        } else if (event->type == CLAP_EVENT_NOTE_OFF) {
          synth.noteOff(noteEvent->key);
          pushDownstreamEvent(
              {.type = DownstreamEventType::HostNote,
               .note = {.noteNumber = noteEvent->key, .velocity = 0.0}});
        }
      }
      if (event->type == CLAP_EVENT_PARAM_VALUE) {
        auto *paramEvent = (const clap_event_param_value_t *)event;
        auto paramId = paramEvent->param_id;
        auto value = paramEvent->value;
        synth.setParameter(paramId, value);
        pushDownstreamEvent(
            {.type = DownstreamEventType::ParameterChange,
             .param = {.paramId = paramId, .value = (float)value}});
      }
    }
  }

  void drainUpstreamEvents(const clap_output_events_t *out) {
    // outgoing parameters, UI --> Host, DSP
    UpstreamEvent item;
    while (eventBridge.popUpstreamEvent(item)) {
      if (item.type == UpstreamEventType::ParameterBeginEdit) {
        clap_event_param_gesture_t clapEvent{};
        mapUpstreamParamGestureEventToClapEvent(item, clapEvent);
        out->try_push(out, &clapEvent.header);
      } else if (item.type == UpstreamEventType::ParameterApplyEdit) {
        synth.setParameter(item.param.paramId, item.param.value);
        clap_event_param_value_t clapEvent{};
        mapUpstreamParamChangeEventToClapEvent(item, clapEvent);
        out->try_push(out, &clapEvent.header);
      } else if (item.type == UpstreamEventType::ParameterEndEdit) {
        clap_event_param_gesture_t clapEvent{};
        mapUpstreamParamGestureEventToClapEvent(item, clapEvent);
        out->try_push(out, &clapEvent.header);
      } else if (item.type == UpstreamEventType::NoteRequest) {
        if (item.note.velocity > 0.f) {
          synth.noteOn(item.note.noteNumber, item.note.velocity);
        } else {
          synth.noteOff(item.note.noteNumber);
        }
        pushDownstreamEvent({.type = DownstreamEventType::HostNote,
                             .note = {.noteNumber = item.note.noteNumber,
                                      .velocity = item.note.velocity}});
      }
    }
  }

public:
  ProcessorAdapter(IPluginSynthesizer &synth, IEventBridge &eventBridge)
      : synth(synth), eventBridge(eventBridge) {}

  void prepareProcessing(double sampleRate, uint32_t maxFrameCount) {
    synth.prepareProcessing(sampleRate, maxFrameCount);
  }

  void renderAudio(uint32_t start, uint32_t end, float *outputL,
                   float *outputR) {
    synth.processAudio(outputL + start, outputR + start, end - start);
  }

  clap_process_status process(const clap_process_t *process) {
    if (!process)
      return CLAP_PROCESS_ERROR;
    if (process->audio_outputs_count != 1)
      return CLAP_PROCESS_ERROR;

    auto &out = process->audio_outputs[0];
    if (out.data64)
      return CLAP_PROCESS_ERROR;
    if (!out.data32 || out.channel_count < 2 || !out.data32[0] ||
        !out.data32[1])
      return CLAP_PROCESS_ERROR;

    drainUpstreamEvents(process->out_events);

    const uint32_t frameCount = process->frames_count;
    const uint32_t inputEventCount =
        process->in_events ? process->in_events->size(process->in_events) : 0;
    uint32_t eventIndex = 0;
    uint32_t nextEventFrame = inputEventCount ? 0 : frameCount;

    for (uint32_t i = 0; i < frameCount;) {
      while (eventIndex < inputEventCount && nextEventFrame == i) {
        const clap_event_header_t *event =
            process->in_events->get(process->in_events, eventIndex);
        if (!event) {
          eventIndex++;
          continue;
        }

        if (event->time != i) {
          nextEventFrame = event->time;
          break;
        }

        this->processInputEvent(event);
        eventIndex++;

        if (eventIndex == inputEventCount) {
          nextEventFrame = frameCount;
          break;
        }
      }

      this->renderAudio(i, nextEventFrame, out.data32[0], out.data32[1]);
      i = nextEventFrame;
    }

    return CLAP_PROCESS_CONTINUE;
  }

  void flushParameters(const clap_input_events_t *in,
                       const clap_output_events_t *out) {
    if (in) {
      for (uint32_t i = 0; i < in->size(in); i++) {
        const clap_event_header_t *event = in->get(in, i);
        this->processInputEvent(event);
      }
    }

    if (out) {
      drainUpstreamEvents(out);
    }
  }
};

} // namespace sonic