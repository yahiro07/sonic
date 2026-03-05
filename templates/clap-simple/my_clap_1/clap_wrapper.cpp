#include "clap_wrapper.h"
#include "clap/factory/plugin-factory.h"
#include "clap/process.h"
#include <assert.h>
#include <cstdio>
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

static const char *const pluginFeatures[] = {
    CLAP_PLUGIN_FEATURE_INSTRUMENT,
    CLAP_PLUGIN_FEATURE_SYNTHESIZER,
    CLAP_PLUGIN_FEATURE_STEREO,
    nullptr,
};

static clap_plugin_descriptor_t pluginDescriptor = {
    .clap_version = CLAP_VERSION_INIT,
    .id = "com.my-company.my-plugin",
    .name = "MyPlugin",
    .vendor = "MyCompany",
    .url = "https://my-company.com",
    .manual_url = "https://my-company.com/manual",
    .support_url = "https://my-company.com/support",
    .version = "1.0.0",
    .description = "Example CLAP plugin.",
    .features = pluginFeatures,
};

static void overwriteDescriptor(const PluginMeta &meta) {
  pluginDescriptor.id = meta.id;
  pluginDescriptor.name = meta.name;
  pluginDescriptor.vendor = meta.vendor;
  pluginDescriptor.url = meta.url;
  pluginDescriptor.manual_url = meta.manualUrl;
  pluginDescriptor.support_url = meta.supportUrl;
  pluginDescriptor.version = meta.version;
  pluginDescriptor.description = meta.description;
}

class PlugBasis {
public:
  clap_plugin_t clapPlugin;
  sonic_common::MacWebView *webView;

  virtual ~PlugBasis() = default;
  virtual void setSampleRate(double sampleRate) = 0;
  virtual clap_process_status process(const clap_process_t *processData) = 0;

  virtual uint32_t getParameterCount() const = 0;
  virtual void getParameterInfo(uint32_t index,
                                clap_param_info_t *info) const = 0;
  virtual double getParameterValue(clap_id id) const = 0;
  virtual void setParameterValue(clap_id id, double value) = 0;
};

static PlugBasis *getPluginData(const clap_plugin_t *plugin) {
  return (PlugBasis *)plugin->plugin_data;
}

static const clap_plugin_params_t extensionParams = {

    .count = [](const clap_plugin_t *plugin) -> uint32_t {
      auto plug = getPluginData(plugin);
      return plug->getParameterCount();
    },

    .get_info = [](const clap_plugin_t *plugin, uint32_t index,
                   clap_param_info_t *info) -> bool {
      auto plug = getPluginData(plugin);
      if (index >= plug->getParameterCount())
        return false;

      plug->getParameterInfo(index, info);
      // printf("get_info %d %s %d %f\n", index, info->name, info->id,
      //        info->default_value);
      return true;
    },

    .get_value = [](const clap_plugin_t *plugin, clap_id id,
                    double *value) -> bool {
      auto plug = getPluginData(plugin);
      *value = plug->getParameterValue(id);
      // printf("get_value %f\n", *value);
      return true;
    },

    .value_to_text = [](const clap_plugin_t *plugin, clap_id id, double value,
                        char *display, uint32_t size) -> bool {
      snprintf(display, size, "%.3f", value);
      return true;
    },

    .text_to_value = [](const clap_plugin_t *plugin, clap_id id,
                        const char *display, double *value) -> bool {
      *value = atof(display);
      return true;
    },

    .flush =
        [](const clap_plugin_t *plugin, const clap_input_events_t *in,
           const clap_output_events_t *out) {
          auto plug = getPluginData(plugin);

          const uint32_t n = in->size(in);
          for (uint32_t i = 0; i < n; ++i) {
            const clap_event_header_t *hdr = in->get(in, i);
            if (hdr->type == CLAP_EVENT_PARAM_VALUE) {
              auto *ev = (const clap_event_param_value_t *)hdr;
              // printf("flush, param %d %f\n", ev->param_id, ev->value);
              plug->setParameterValue(ev->param_id, ev->value);
            }
          }
        }};

#define GUI_API CLAP_WINDOW_API_COCOA

static const clap_plugin_gui_t extensionGUI = {
    .is_api_supported = [](const clap_plugin_t *plugin, const char *api,
                           bool isFloating) -> bool {
      return 0 == strcmp(api, GUI_API) && !isFloating;
    },

    .get_preferred_api = [](const clap_plugin_t *plugin, const char **api,
                            bool *isFloating) -> bool {
      *api = GUI_API;
      *isFloating = false;
      return true;
    },

    .create = [](const clap_plugin_t *_plugin, const char *api,
                 bool isFloating) -> bool {
      if (!extensionGUI.is_api_supported(_plugin, api, isFloating))
        return false;
      auto plug = getPluginData(_plugin);
      plug->webView = new sonic_common::MacWebView();
      return true;
    },

    .destroy =
        [](const clap_plugin_t *_plugin) {
          auto plug = getPluginData(_plugin);
          if (plug->webView) {
            delete plug->webView;
            plug->webView = nullptr;
          }
        },

    .set_scale = [](const clap_plugin_t *plugin, double scale) -> bool {
      return false;
    },

    .get_size = [](const clap_plugin_t *plugin, uint32_t *width,
                   uint32_t *height) -> bool {
      *width = 800;
      *height = 600;
      return true;
    },

    .can_resize = [](const clap_plugin_t *plugin) -> bool { return false; },

    .get_resize_hints = [](const clap_plugin_t *plugin,
                           clap_gui_resize_hints_t *hints) -> bool {
      return false;
    },

    .adjust_size = [](const clap_plugin_t *plugin, uint32_t *width,
                      uint32_t *height) -> bool {
      return extensionGUI.get_size(plugin, width, height);
    },

    .set_size = [](const clap_plugin_t *plugin, uint32_t width,
                   uint32_t height) -> bool { return true; },

    .set_parent = [](const clap_plugin_t *_plugin,
                     const clap_window_t *window) -> bool {
      assert(0 == strcmp(window->api, GUI_API));
      auto plug = getPluginData(_plugin);
      plug->webView->attachToParent(window->cocoa);
      return true;
    },

    .set_transient = [](const clap_plugin_t *plugin,
                        const clap_window_t *window) -> bool { return false; },

    .suggest_title = [](const clap_plugin_t *plugin, const char *title) {},

    .show = [](const clap_plugin_t *_plugin) -> bool {
      auto plug = getPluginData(_plugin);
      // plug->webView->show();
      return true;
    },

    .hide = [](const clap_plugin_t *_plugin) -> bool {
      auto plug = getPluginData(_plugin);
      plug->webView->removeFromParent();
      return true;
    },
};

static const clap_plugin_note_ports_t extensionNotePorts = {
    .count = [](const clap_plugin_t *plugin, bool isInput) -> uint32_t {
      return isInput ? 1 : 0;
    },

    .get = [](const clap_plugin_t *plugin, uint32_t index, bool isInput,
              clap_note_port_info_t *info) -> bool {
      if (!isInput || index)
        return false;
      info->id = 0;
      info->supported_dialects = CLAP_NOTE_DIALECT_CLAP;
      info->preferred_dialect = CLAP_NOTE_DIALECT_CLAP;
      snprintf(info->name, sizeof(info->name), "%s", "Note Port");
      return true;
    },
};

static const clap_plugin_audio_ports_t extensionAudioPorts = {
    .count = [](const clap_plugin_t *plugin, bool isInput) -> uint32_t {
      return isInput ? 0 : 1;
    },

    .get = [](const clap_plugin_t *plugin, uint32_t index, bool isInput,
              clap_audio_port_info_t *info) -> bool {
      if (isInput || index)
        return false;
      info->id = 0;
      info->channel_count = 2;
      info->flags = CLAP_AUDIO_PORT_IS_MAIN;
      info->port_type = CLAP_PORT_STEREO;
      info->in_place_pair = CLAP_INVALID_ID;
      snprintf(info->name, sizeof(info->name), "%s", "Audio Output");
      return true;
    },
};

static const clap_plugin_t pluginClass = {
    .desc = &pluginDescriptor,
    .plugin_data = nullptr,

    .init = [](const clap_plugin *_plugin) -> bool {
      auto plug = getPluginData(_plugin);
      (void)plug;
      return true;
    },

    .destroy =
        [](const clap_plugin *_plugin) {
          auto plug = getPluginData(_plugin);
          delete plug;
        },

    .activate = [](const clap_plugin *_plugin, double sampleRate,
                   uint32_t minimumFramesCount,
                   uint32_t maximumFramesCount) -> bool {
      auto plug = getPluginData(_plugin);
      plug->setSampleRate(sampleRate);
      return true;
    },

    .deactivate = [](const clap_plugin *_plugin) {},

    .start_processing = [](const clap_plugin *_plugin) -> bool { return true; },

    .stop_processing = [](const clap_plugin *_plugin) {},

    .reset =
        [](const clap_plugin *_plugin) { auto plug = getPluginData(_plugin); },

    .process = [](const clap_plugin *_plugin,
                  const clap_process_t *processData) -> clap_process_status {
      auto plug = getPluginData(_plugin);
      return plug->process(processData);
    },

    .get_extension = [](const clap_plugin *plugin,
                        const char *id) -> const void * {
      if (0 == strcmp(id, CLAP_EXT_NOTE_PORTS))
        return &extensionNotePorts;
      if (0 == strcmp(id, CLAP_EXT_AUDIO_PORTS))
        return &extensionAudioPorts;
      if (!strcmp(id, CLAP_EXT_PARAMS))
        return &extensionParams;
      if (!strcmp(id, CLAP_EXT_GUI))
        return &extensionGUI;
      return nullptr;
    },

    .on_main_thread = [](const clap_plugin *_plugin) {},

};

typedef PlugBasis *(*createPLugBasisInstanceFn)(SynthesizerBase *synth);

static struct ClapRootage {
  SynthesizerInitializerFn synthInitializer;
  createPLugBasisInstanceFn createPlugBasisInstance;
} rootage;

static const clap_plugin_factory_t pluginFactory = {
    .get_plugin_count = [](const clap_plugin_factory *factory) -> uint32_t {
      return 1;
    },

    .get_plugin_descriptor =
        [](const clap_plugin_factory *factory,
           uint32_t index) -> const clap_plugin_descriptor_t * {
      return index == 0 ? &pluginDescriptor : nullptr;
    },

    .create_plugin = [](const clap_plugin_factory *factory,
                        const clap_host_t *host,
                        const char *pluginID) -> const clap_plugin_t * {
      if (!clap_version_is_compatible(host->clap_version) ||
          strcmp(pluginID, pluginDescriptor.id)) {
        return nullptr;
      }

      auto synth = rootage.synthInitializer();
      PlugBasis *plugBasis = rootage.createPlugBasisInstance(synth);
      plugBasis->clapPlugin = pluginClass;
      plugBasis->clapPlugin.plugin_data = plugBasis;
      return &plugBasis->clapPlugin;
    },
};

static const clap_plugin_entry_t clapEntry = {
    .clap_version = CLAP_VERSION_INIT,
    .init = [](const char *path) -> bool { return true; },
    .deinit = []() {},
    .get_factory = [](const char *factoryID) -> const void * {
      return strcmp(factoryID, CLAP_PLUGIN_FACTORY_ID) ? nullptr
                                                       : &pluginFactory;
    },
};

class PlugBasisImpl : public PlugBasis {
private:
  const clap_host_t *clapHost;
  PlugDriver plugDriver;

public:
  PlugBasisImpl(SynthesizerBase &synth) : plugDriver(&synth) {}

  void setSampleRate(double sampleRate) override {
    plugDriver.setSampleRate(sampleRate);
  }

  clap_process_status process(const clap_process_t *processData) override {
    plugDriver.process(processData);
    return CLAP_PROCESS_CONTINUE;
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
};

PlugBasis *createPlugBasisInstance(SynthesizerBase *synth) {
  return new PlugBasisImpl(*synth);
}

const clap_plugin_entry_t &
createClapPluginEntry(SynthesizerInitializerFn synthInitializer,
                      const PluginMeta &meta) {
  rootage.synthInitializer = synthInitializer;
  rootage.createPlugBasisInstance = createPlugBasisInstance;

  overwriteDescriptor(meta);

  return clapEntry;
}