#include "clap_wrapper.h"
#include <assert.h>
#include <cstdio>
#include <cstring>
#include <memory>

class PlugDriver {
private:
  const clap_host_t *claHost;
  std::unique_ptr<SynthesizerBase> synth;

public:
  clap_plugin_t clapPlugin;

  PlugDriver(const clap_host_t *claHost, SynthesizerBase *synth)
      : claHost(claHost), synth(synth) {}

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