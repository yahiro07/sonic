#pragma once

#include "./messaging_hub.h"
#include "clap/clap.h"
#include "sonic_common/general/spsc_queue.h"
#include "sonic_common/synthesizer_base.h"

static void mapUpstreamEventToClapEvent(UpstreamEvent &upstreamEvent,
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

class ProcessorAdapter {
private:
  SynthesizerBase *synth;
  const clap_host_t *host;
  const clap_host_params_t *hostParams = nullptr;
  std::atomic<bool> downstreamDrainRequested{false};
  sonic_common::SPSCQueue<UpstreamEvent, 32> upstreamEventQueue;
  sonic_common::SPSCQueue<DownstreamEvent, 32> downstreamEventQueue;

public:
  ProcessorAdapter(SynthesizerBase *synth) : synth(synth) {}

  void initialize(const clap_host_t *host,
                  const clap_host_params_t *hostParams) {
    this->host = host;
    this->hostParams = hostParams;
  }

  void setSampleRate(double sampleRate) { synth->setSampleRate(sampleRate); }

  void renderAudio(uint32_t start, uint32_t end, float *outputL,
                   float *outputR) {
    synth->processAudio(outputL + start, outputR + start, end - start);
  }

  void requestDownstreamDrainOnMainThread() noexcept {
    if (!this->host || !this->host->request_callback) {
      return;
    }
    bool expected = false;
    if (downstreamDrainRequested.compare_exchange_strong(
            expected, true, std::memory_order_acq_rel)) {
      this->host->request_callback(this->host);
    }
  }

  void pushDownstreamEvent(DownstreamEvent e) {
    downstreamEventQueue.push(e);
    requestDownstreamDrainOnMainThread();
  }

  void processInputEvent(const clap_event_header_t *event) {
    if (event->space_id == CLAP_CORE_EVENT_SPACE_ID) {
      if (event->type == CLAP_EVENT_NOTE_ON ||
          event->type == CLAP_EVENT_NOTE_OFF) {
        const clap_event_note_t *noteEvent = (const clap_event_note_t *)event;
        if (event->type == CLAP_EVENT_NOTE_ON) {
          synth->noteOn(noteEvent->key, noteEvent->velocity);
          pushDownstreamEvent({.type = DownStreamEventType::hostNoteOn,
                               .note = {.noteNumber = noteEvent->key,
                                        .velocity = noteEvent->velocity}});
        } else if (event->type == CLAP_EVENT_NOTE_OFF) {
          synth->noteOff(noteEvent->key);
          pushDownstreamEvent({.type = DownStreamEventType::hostNoteOff,
                               .note = {.noteNumber = noteEvent->key}});
        }
      }
      if (event->type == CLAP_EVENT_PARAM_VALUE) {
        auto *paramEvent = (const clap_event_param_value_t *)event;
        // printf("processEvent, param in %d %f\n", paramEvent->param_id,
        //        paramEvent->value);
        auto paramId = paramEvent->param_id;
        auto value = paramEvent->value;
        synth->setParameter(paramId, value);
        pushDownstreamEvent({.type = DownStreamEventType::parameterChange,
                             .param = {.paramId = paramId, .value = value}});
      }
    }
  }

  void drainUpstreamEvents(const clap_output_events_t *out) {
    // outgoing parameters, UI --> Host, DSP
    UpstreamEvent item;
    while (upstreamEventQueue.pop(item)) {
      if (item.type == UpStreamEventType::parameterApplyEdit) {
        synth->setParameter(item.param.paramId, item.param.value);
        clap_event_param_value_t clapEvent{};
        mapUpstreamEventToClapEvent(item, clapEvent);
        out->try_push(out, &clapEvent.header);
      } else if (item.type == UpStreamEventType::noteOnRequest) {
        synth->noteOn(item.note.noteNumber, item.note.velocity);
      } else if (item.type == UpStreamEventType::noteOffRequest) {
        synth->noteOff(item.note.noteNumber);
      }
    }
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
    printf("flushParameters\n");

    for (uint32_t i = 0; i < in->size(in); i++) {
      const clap_event_header_t *event = in->get(in, i);
      this->processInputEvent(event);
    }

    drainUpstreamEvents(out);
  }

  void pushUpstreamEvent(UpstreamEvent &e) {
    this->upstreamEventQueue.push(e);
    if (this->hostParams) {
      this->hostParams->request_flush(this->host);
    }
  }

  bool popDownstreamEvent(DownstreamEvent &e) {
    if (downstreamDrainRequested) {
      downstreamDrainRequested.store(false, std::memory_order_release);
    }
    return downstreamEventQueue.pop(e);
  }
};
