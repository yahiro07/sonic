#include "./clap_wrapper.h"
#include "../synthesizer_base.h"
#include "./clap_rootage.h"
#include "clap/entry.h"
#include "clap/plugin.h"
#include "clap/process.h"
#include "sonic_common/general/mac_web_view.h"
#include <assert.h>
#include <cstring>
#include <memory>
#include <string>

class PlugDriver {

public:
  std::unique_ptr<SynthesizerBase> synth;

  PlugDriver(SynthesizerBase *synth) : synth(synth) {}

  void setSampleRate(double sampleRate) { synth->setSampleRate(sampleRate); }

  void renderAudio(uint32_t start, uint32_t end, float *outputL,
                   float *outputR) {
    synth->processAudio(outputL + start, outputR + start, end - start);
  }

  void processEvent(const clap_event_header_t *event) {
    if (event->space_id == CLAP_CORE_EVENT_SPACE_ID) {
      if (event->type == CLAP_EVENT_NOTE_ON ||
          event->type == CLAP_EVENT_NOTE_OFF) {
        const clap_event_note_t *noteEvent = (const clap_event_note_t *)event;
        if (event->type == CLAP_EVENT_NOTE_ON) {
          synth->noteOn(noteEvent->key, noteEvent->velocity);
        } else if (event->type == CLAP_EVENT_NOTE_OFF) {
          synth->noteOff(noteEvent->key);
        }
      }
      if (event->type == CLAP_EVENT_PARAM_VALUE) {
        auto *paramEvent = (const clap_event_param_value_t *)event;
        // printf("param %d %f\n", paramEvent->param_id, paramEvent->value);
        synth->setParameterValue(paramEvent->param_id, paramEvent->value);
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

        processEvent(event);
        eventIndex++;

        if (eventIndex == inputEventCount) {
          nextEventFrame = frameCount;
          break;
        }
      }

      renderAudio(i, nextEventFrame, out.data32[0], out.data32[1]);
      i = nextEventFrame;
    }

    return CLAP_PROCESS_CONTINUE;
  }
};

#define GUI_API CLAP_WINDOW_API_COCOA

class PlugBasisImpl : public PlugBasis {
private:
  const clap_host_t *clapHost = nullptr;
  PlugDriver plugDriver;
  sonic_common::MacWebView *webView = nullptr;

public:
  PlugBasisImpl(SynthesizerBase &synth) : plugDriver(&synth) {}

  void setSampleRate(double sampleRate) override {
    plugDriver.setSampleRate(sampleRate);
  }

  clap_process_status process(const clap_process_t *processData) override {
    return plugDriver.process(processData);
  }

  uint32_t getParameterCount() const override {
    return plugDriver.synth->getParameterCount();
  }

  void getParameterInfo(uint32_t index,
                        clap_param_info_t *info) const override {
    plugDriver.synth->getParameterInfo(index, info);
  }

  double getParameterValue(clap_id id) const override {
    return plugDriver.synth->getParameterValue(id);
  }

  void setParameterValue(clap_id id, double value) override {
    plugDriver.synth->setParameterValue(id, value);
  }

  bool guiCreate() override {
    webView = new sonic_common::MacWebView();
    webView->loadUrl("http://localhost:3000");

    // todo: delegate event handing to messaging hub
    webView->setMessageReceiver([](const std::string &message) {
      printf("message: %s\n", message.c_str());
    });
    return true;
  }

  void guiDestroy() override {
    webView->setMessageReceiver(nullptr);
    webView->removeFromParent();
    if (webView) {
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