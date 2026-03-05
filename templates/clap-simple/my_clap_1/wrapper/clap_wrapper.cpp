#include "./clap_wrapper.h"
#include "./clap_rootage.h"
#include <assert.h>
#include <clap/process.h>
#include <cstring>
#include <memory>
#include <sonic_common/general/mac_web_view.h>

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
    return true;
  }

  void guiDestroy() override {
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

  bool guiHide() override {
    webView->removeFromParent();
    return true;
  }
};

PlugBasis *createPlugBasisInstance(SynthesizerBase *synth) {
  return new PlugBasisImpl(*synth);
}

static void overwriteDescriptor(const PluginMeta &meta) {
  auto &pluginDescriptor = clapRootage_getPluginDescriptor();
  pluginDescriptor.id = meta.id;
  pluginDescriptor.name = meta.name;
  pluginDescriptor.vendor = meta.vendor;
  pluginDescriptor.url = meta.url;
  pluginDescriptor.manual_url = meta.manualUrl;
  pluginDescriptor.support_url = meta.supportUrl;
  pluginDescriptor.version = meta.version;
  pluginDescriptor.description = meta.description;
}

const clap_plugin_entry_t &
createClapPluginEntry(SynthesizerInitializerFn synthInitializer,
                      const PluginMeta &meta) {
  clapRootage_setPlugBasisInstantiateFn([synthInitializer]() {
    auto synth = synthInitializer();
    return new PlugBasisImpl(*synth);
  });

  overwriteDescriptor(meta);

  return clapRootage_getClapPluginEntry();
}