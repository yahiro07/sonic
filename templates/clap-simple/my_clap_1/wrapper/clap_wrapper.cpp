#include "./clap_wrapper.h"
#include "../rootage/clap_rootage.h"
#include "clap/entry.h"
#include "clap/events.h"
#include "clap/plugin.h"
#include "clap/process.h"
#include "messaging_hub.h"
#include "sonic_common/general/mac_web_view.h"
#include "sonic_common/general/polling_timer.h"
#include "sonic_common/general/spsc_queue.h"
#include "sonic_common/logic/parameter_builder_impl.h"
#include "sonic_common/logic/parameter_definitions_provider.h"
#include <assert.h>
#include <cstring>
#include <memory>
#include <string>

#define GUI_API CLAP_WINDOW_API_COCOA

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

class PlugBasisImpl : public PlugBasis {
private:
  std::unique_ptr<SynthesizerBase> synth;

  sonic_common::MacWebView *webView = nullptr;
  sonic_common::SPSCQueue<UpstreamEvent, 32> upstreamEventQueue;
  sonic_common::SPSCQueue<DownstreamEvent, 32> downstreamEventQueue;
  sonic_common::PollingTimer pollingTimer;
  sonic_common::ParameterDefinitionsProvider parameterDefinitionsProvider;

  std::unordered_map<uint32_t, double> parameterValuesInMainThread;

private:
  void renderAudio(uint32_t start, uint32_t end, float *outputL,
                   float *outputR) {
    synth->processAudio(outputL + start, outputR + start, end - start);
  }

  void processInputEvent(const clap_event_header_t *event) {
    if (event->space_id == CLAP_CORE_EVENT_SPACE_ID) {
      if (event->type == CLAP_EVENT_NOTE_ON ||
          event->type == CLAP_EVENT_NOTE_OFF) {
        const clap_event_note_t *noteEvent = (const clap_event_note_t *)event;
        if (event->type == CLAP_EVENT_NOTE_ON) {
          synth->noteOn(noteEvent->key, noteEvent->velocity);
          downstreamEventQueue.push(
              {.type = DownStreamEventType::hostNoteOn,
               .note = {.noteNumber = noteEvent->key,
                        .velocity = noteEvent->velocity}});
        } else if (event->type == CLAP_EVENT_NOTE_OFF) {
          synth->noteOff(noteEvent->key);
          downstreamEventQueue.push({.type = DownStreamEventType::hostNoteOff,
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
        downstreamEventQueue.push(
            {.type = DownStreamEventType::parameterChange,
             .param = {.paramId = paramId, .value = value}});
      }
    }
  }

public:
  PlugBasisImpl(SynthesizerBase &synth) : synth(&synth) {
    auto parameterBuilder = sonic_common::ParameterBuilderImpl();
    synth.setupParameters(parameterBuilder);
    auto parameterItems = parameterBuilder.getItems();
    uint64_t maxAddress = 0xFFFFFFFE; // for valid clap_id
    parameterDefinitionsProvider.addParameters(parameterItems, maxAddress);
    for (auto &item : parameterItems) {
      synth.setParameter(item.address, item.defaultValue);
      parameterValuesInMainThread[item.address] = item.defaultValue;
    }
  }

  void setSampleRate(double sampleRate) override {
    synth->setSampleRate(sampleRate);
  }

  clap_process_status process(const clap_process_t *process) override {
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

    // outgoing parameters, UI --> Host, DSP
    UpstreamEvent item;
    while (upstreamEventQueue.pop(item)) {
      if (item.type == UpStreamEventType::parameterApplyEdit) {
        synth->setParameter(item.param.paramId, item.param.value);
        clap_event_param_value_t clapEvent{};
        mapUpstreamEventToClapEvent(item, clapEvent);
        process->out_events->try_push(process->out_events, &clapEvent.header);
      }
    }

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

  uint32_t getParameterCount() const override {
    return parameterDefinitionsProvider.getParameterItems().size();
  }

  void getParameterInfo(uint32_t index,
                        clap_param_info_t *info) const override {
    auto parameterItems = parameterDefinitionsProvider.getParameterItems();
    auto &item = parameterItems[index];
    info->id = item.address;
    info->flags = mapParameterFlags(item.flags);
    info->min_value = item.minValue;
    info->max_value = item.maxValue;
    info->default_value = item.defaultValue;
    snprintf(info->name, sizeof(info->name), "%s", item.label.c_str());
  }

  double getParameterValue(clap_id id) const override {
    auto kv = parameterValuesInMainThread.find(id);
    if (kv == parameterValuesInMainThread.end()) {
      return 0.0;
    }
    return kv->second;
  }

  void flushParameters(const clap_input_events_t *in,
                       const clap_output_events_t *out) override {
    printf("flushParameters\n");

    for (uint32_t i = 0; i < in->size(in); i++) {
      const clap_event_header_t *event = in->get(in, i);
      this->processInputEvent(event);
    }

    // outgoing parameters, UI --> Host, DSP
    UpstreamEvent item;
    while (upstreamEventQueue.pop(item)) {
      if (item.type == UpStreamEventType::parameterApplyEdit) {
        synth->setParameter(item.param.paramId, item.param.value);
        clap_event_param_value_t clapEvent{};
        mapUpstreamEventToClapEvent(item, clapEvent);
        out->try_push(out, &clapEvent.header);
      }
    }
  }

  void drainDownstreamEvents() {

    DownstreamEvent e;
    while (downstreamEventQueue.pop(e)) {
      if (e.type == DownStreamEventType::parameterChange) {
        parameterValuesInMainThread[e.param.paramId] = e.param.value;
      }
      messagingHub_dev_handleEventFromHost(
          e,
          [this](std::string &message) { this->webView->sendMessage(message); },
          parameterDefinitionsProvider);
    }
  }

  bool guiCreate() override {
    webView = new sonic_common::MacWebView();
    webView->loadUrl("http://localhost:3000");

    webView->setMessageReceiver([this](const std::string &message) {
      auto performParameterEditFromUi = [this](std::string &identifier,
                                               double value) {
        printf("setParameterFromUi: %s %f\n", identifier.c_str(), value);
        auto _address =
            parameterDefinitionsProvider.getAddressByIdentifier(identifier);
        if (!_address)
          return;
        auto paramId = static_cast<uint32_t>(*_address);
        auto kv = parameterValuesInMainThread.find(paramId);
        if (kv != parameterValuesInMainThread.end()) {
          kv->second = value;
        }
        this->upstreamEventQueue.push(
            {.type = UpStreamEventType::parameterApplyEdit,
             .param = {
                 .paramId = paramId,
                 .value = value,
             }});
        if (this->hostParams) {
          this->hostParams->request_flush(this->host);
        }
      };
      auto emitUpstreamEvent = [this](UpstreamEvent &event) {
        this->upstreamEventQueue.push(event);
        if (this->hostParams) {
          this->hostParams->request_flush(this->host);
        }
      };
      messagingHub_dev_handleMessageFromUi(message, performParameterEditFromUi,
                                           emitUpstreamEvent);
    });
    pollingTimer.start([this]() { this->drainDownstreamEvents(); }, 50);
    return true;
  }

  void guiDestroy() override {
    pollingTimer.stop();
    if (webView) {
      webView->setMessageReceiver(nullptr);
      webView->removeFromParent();
      delete webView;
      webView = nullptr;
    }
  }

  bool guiSetParent(const clap_window_t *window) override {
    assert(0 == strcmp(window->api, GUI_API));
    webView->attachToParent(window->cocoa);
    return true;
  }

  bool guiSetSize(uint32_t width, uint32_t height) override {
    webView->setFrame(0, 0, width, height);
    return true;
  }

  // bool guiSetTransient(const clap_window_t *window) override { return false;
  // } void guiSuggestTitle(const char *title) override {}
  bool guiShow() override {
    // webView->show();
    return true;
  }

  bool guiHide() override { return true; }
};

static void overwriteDescriptor(clap_plugin_descriptor_t &desc,
                                const PluginMeta &meta) {
  desc.id = meta.id;
  desc.name = meta.name;
  desc.vendor = meta.vendor;
  desc.url = meta.url;
  desc.manual_url = meta.manualUrl;
  desc.support_url = meta.supportUrl;
  desc.version = meta.version;
  desc.description = meta.description;
}

const clap_plugin_entry_t &
createClapPluginEntry(SynthesizerInitializerFn synthInitializer,
                      const PluginMeta &meta) {
  static const clap_plugin_entry_t clapPlugin = [synthInitializer, meta]() {
    auto desc = clapRootage_getPluginDescriptor();
    overwriteDescriptor(desc, meta);
    clapRootage_setPluginBasisInstantiateFn([synthInitializer]() {
      auto synth = synthInitializer();
      return new PlugBasisImpl(*synth);
    });
    return clapRootage_getClapPluginEntry();
  }();
  return clapPlugin;
}