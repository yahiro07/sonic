#include "clap_wrapper.h"
#include <assert.h>
#include <cstdio>
#include <cstring>
#include <memory>
#include <sonic_common/general/mac_web_view.h>

class PlugDriver {
private:
  const clap_host_t *clapHost;

public:
  std::unique_ptr<SynthesizerBase> synth;
  clap_plugin_t clapPlugin;

  sonic_common::MacWebView *webView;

  PlugDriver(const clap_host_t *clapHost, SynthesizerBase *synth)
      : clapHost(clapHost), synth(synth) {}

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

static const clap_plugin_params_t extensionParams = {

    .count = [](const clap_plugin_t *plugin) -> uint32_t {
      auto drv = (PlugDriver *)plugin->plugin_data;
      return drv->synth->getParameterCount();
    },

    .get_info = [](const clap_plugin_t *plugin, uint32_t index,
                   clap_param_info_t *info) -> bool {
      auto drv = (PlugDriver *)plugin->plugin_data;
      if (index >= drv->synth->getParameterCount())
        return false;

      drv->synth->getParameterInfo(index, info);
      // printf("get_info %d %s %d %f\n", index, info->name, info->id,
      //        info->default_value);
      return true;
    },

    .get_value = [](const clap_plugin_t *plugin, clap_id id,
                    double *value) -> bool {
      auto drv = (PlugDriver *)plugin->plugin_data;
      *value = drv->synth->getParameterValue(id);
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
          auto drv = (PlugDriver *)plugin->plugin_data;

          const uint32_t n = in->size(in);
          for (uint32_t i = 0; i < n; ++i) {
            const clap_event_header_t *hdr = in->get(in, i);
            if (hdr->type == CLAP_EVENT_PARAM_VALUE) {
              auto *ev = (const clap_event_param_value_t *)hdr;
              // printf("flush, param %d %f\n", ev->param_id, ev->value);
              drv->synth->setParameterValue(ev->param_id, ev->value);
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
      auto drv = (PlugDriver *)_plugin->plugin_data;
      drv->webView = new sonic_common::MacWebView();
      return true;
    },

    .destroy =
        [](const clap_plugin_t *_plugin) {
          auto drv = (PlugDriver *)_plugin->plugin_data;
          if (drv->webView) {
            delete drv->webView;
            drv->webView = nullptr;
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
      auto drv = (PlugDriver *)_plugin->plugin_data;
      drv->webView->attachToParent(window->cocoa);
      return true;
    },

    .set_transient = [](const clap_plugin_t *plugin,
                        const clap_window_t *window) -> bool { return false; },

    .suggest_title = [](const clap_plugin_t *plugin, const char *title) {},

    .show = [](const clap_plugin_t *_plugin) -> bool {
      auto drv = (PlugDriver *)_plugin->plugin_data;
      // drv->webView->show();
      return true;
    },

    .hide = [](const clap_plugin_t *_plugin) -> bool {
      auto drv = (PlugDriver *)_plugin->plugin_data;
      drv->webView->removeFromParent();
      return true;
    },
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
      auto plugDriver = (PlugDriver *)_plugin->plugin_data;
      (void)plugDriver;
      return true;
    },

    .destroy =
        [](const clap_plugin *_plugin) {
          auto plugDriver = (PlugDriver *)_plugin->plugin_data;
          delete plugDriver;
        },

    .activate = [](const clap_plugin *_plugin, double sampleRate,
                   uint32_t minimumFramesCount,
                   uint32_t maximumFramesCount) -> bool {
      auto plugDriver = (PlugDriver *)_plugin->plugin_data;
      plugDriver->setSampleRate(sampleRate);
      return true;
    },

    .deactivate = [](const clap_plugin *_plugin) {},

    .start_processing = [](const clap_plugin *_plugin) -> bool { return true; },

    .stop_processing = [](const clap_plugin *_plugin) {},

    .reset =
        [](const clap_plugin *_plugin) {
          auto plugDriver = (PlugDriver *)_plugin->plugin_data;
        },

    .process = [](const clap_plugin *_plugin,
                  const clap_process_t *process) -> clap_process_status {
      auto plugDriver = (PlugDriver *)_plugin->plugin_data;
      return plugDriver->process(process);
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

static struct ClapRootage {
  synthesizerInitializerFn synthInitializer;
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
      PlugDriver *plugDriver = new PlugDriver(host, synth);
      plugDriver->clapPlugin = pluginClass;
      plugDriver->clapPlugin.plugin_data = plugDriver;
      return &plugDriver->clapPlugin;
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

const clap_plugin_entry_t &
createClapPluginEntry(synthesizerInitializerFn synthInitializer,
                      const PluginMeta &meta) {
  rootage.synthInitializer = synthInitializer;
  overwriteDescriptor(meta);
  return clapEntry;
}