#include "plugin_bridge.h"
#include <assert.h>
#include <clap/clap.h>
#include <dlfcn.h>
#include <string>
#include <vector>

namespace clap_dev_host {

struct PluginBridge::PluginBridgeInternal {
  void *handle = nullptr;
  const clap_plugin_entry_t *entry = nullptr;
  const clap_plugin_factory_t *factory = nullptr;
  const clap_plugin_t *plugin = nullptr;
  clap_host_t host;

  // Extensions
  const clap_plugin_gui_t *plugin_gui = nullptr;
  const clap_plugin_params_t *plugin_params = nullptr;

  std::function<bool(int, int)> editor_resize_callback;
  std::function<void(uint32_t, double)> param_edit_callback;

  PluginBridgeInternal() {
    host.clap_version = CLAP_VERSION_INIT;
    host.host_data = this;
    host.name = "ClapDevHost";
    host.vendor = "sonic";
    host.url = "https://github.com/yahiro07/sonic";
    host.version = "0.0.1";
    host.get_extension = [](const clap_host_t *h,
                            const char *id) -> const void * {
      return static_cast<PluginBridge::PluginBridgeInternal *>(h->host_data)
          ->get_extension(id);
    };
    host.request_restart = [](const clap_host_t *h) {};
    host.request_process = [](const clap_host_t *h) {};
    host.request_callback = [](const clap_host_t *h) {};
  }

  const void *get_extension(const char *id) {
    if (std::string(id) == CLAP_EXT_GUI) {
      static const clap_host_gui_t host_gui = {
          [](const clap_host_t *h) -> void { /* resize_hints_changed */ },
          [](const clap_host_t *h, uint32_t width, uint32_t height) -> bool {
            auto *self =
                static_cast<PluginBridge::PluginBridgeInternal *>(h->host_data);
            if (self->editor_resize_callback) {
              return self->editor_resize_callback(width, height);
            }
            return false;
          },
          [](const clap_host_t *h) -> bool { return true; }, // request_show
          [](const clap_host_t *h) -> bool { return true; }, // request_hide
          [](const clap_host_t *h, bool was_destroyed) {}    // closed
      };
      return &host_gui;
    }
    if (std::string(id) == CLAP_EXT_PARAMS) {
      static const clap_host_params_t host_params = {
          [](const clap_host_t *h, clap_param_rescan_flags flags) {},
          [](const clap_host_t *h, clap_id param_id,
             clap_param_clear_flags flags) {},
          [](const clap_host_t *h) { /* request_flush */ }};
      return &host_params;
    }
    return nullptr;
  }
};

PluginBridge::PluginBridge()
    : states(std::make_unique<PluginBridge::PluginBridgeInternal>()) {}

PluginBridge::~PluginBridge() { unloadPlugin(); }

bool PluginBridge::loadPlugin(const std::string &path) {
  states->handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (!states->handle)
    return false;

  states->entry =
      (const clap_plugin_entry_t *)dlsym(states->handle, "clap_entry");
  if (!states->entry)
    return false;

  if (!states->entry->init(path.c_str()))
    return false;

  states->factory = (const clap_plugin_factory_t *)states->entry->get_factory(
      CLAP_PLUGIN_FACTORY_ID);
  if (!states->factory)
    return false;

  if (states->factory->get_plugin_count(states->factory) == 0)
    return false;

  const clap_plugin_descriptor_t *desc =
      states->factory->get_plugin_descriptor(states->factory, 0);
  states->plugin =
      states->factory->create_plugin(states->factory, &states->host, desc->id);
  if (!states->plugin)
    return false;

  if (!states->plugin->init(states->plugin))
    return false;

  states->plugin_gui = (const clap_plugin_gui_t *)states->plugin->get_extension(
      states->plugin, CLAP_EXT_GUI);
  states->plugin_params =
      (const clap_plugin_params_t *)states->plugin->get_extension(
          states->plugin, CLAP_EXT_PARAMS);

  return true;
}

void PluginBridge::unloadPlugin() {
  if (states->plugin) {
    states->plugin->destroy(states->plugin);
    states->plugin = nullptr;
  }
  if (states->entry) {
    states->entry->deinit();
    states->entry = nullptr;
  }
  if (states->handle) {
    dlclose(states->handle);
    states->handle = nullptr;
  }
}

void PluginBridge::getDesiredEditorSize(int &width, int &height) {
  if (states->plugin_gui) {
    uint32_t w, h;
    if (states->plugin_gui->get_size(states->plugin, &w, &h)) {
      width = w;
      height = h;
    }
  }
}

void PluginBridge::openEditor(void *ownerViewHandle) {
  if (states->plugin_gui) {
    clap_window_t window;
    window.api = CLAP_WINDOW_API_COCOA;
    window.cocoa = ownerViewHandle;

    if (states->plugin_gui->is_api_supported(states->plugin,
                                             CLAP_WINDOW_API_COCOA, false)) {
      states->plugin_gui->create(states->plugin, CLAP_WINDOW_API_COCOA, false);
      states->plugin_gui->set_parent(states->plugin, &window);
      states->plugin_gui->show(states->plugin);
    }
  }
}

void PluginBridge::closeEditor() {
  if (states->plugin_gui) {
    states->plugin_gui->hide(states->plugin);
    states->plugin_gui->destroy(states->plugin);
  }
}

bool PluginBridge::requestEditorResize(int &width, int &height) {
  if (states->plugin_gui) {
    uint32_t w = width;
    uint32_t h = height;
    if (states->plugin_gui->can_resize(states->plugin) &&
        states->plugin_gui->adjust_size(states->plugin, &w, &h)) {
      states->plugin_gui->set_size(states->plugin, w, h);
      width = w;
      height = h;
      return true;
    }
  }
  return false;
}

void PluginBridge::subscribeEditorSizeChangeRequest(
    std::function<bool(int, int)> callback) {
  states->editor_resize_callback = callback;
}

void PluginBridge::unsubscribeEditorSizeChangeRequest() {
  states->editor_resize_callback = nullptr;
}

void PluginBridge::prepareAudio(double sampleRate, int maxBlockSize) {
  if (states->plugin) {
    states->plugin->activate(states->plugin, sampleRate, 1, maxBlockSize);
    states->plugin->start_processing(states->plugin);
  }
}

void PluginBridge::processAudio(float *bufferL, float *bufferR, int nframes,
                                const InputEvent *events, size_t eventCount) {
  if (!states->plugin)
    return;

  float *data[2] = {bufferL, bufferR};
  clap_audio_buffer_t audio_out;
  audio_out.data32 = data;
  audio_out.data64 = nullptr;
  audio_out.channel_count = 2;
  audio_out.latency = 0;
  audio_out.constant_mask = 0;

  struct InternalEventList {
    std::vector<const clap_event_header_t *> headers;
    std::vector<clap_event_note_t> notes;
    std::vector<clap_event_param_value_t> params;

    static uint32_t size(const clap_input_events_t *list) {
      return static_cast<InternalEventList *>(list->ctx)->headers.size();
    }
    static const clap_event_header_t *get(const clap_input_events_t *list,
                                          uint32_t index) {
      return static_cast<InternalEventList *>(list->ctx)->headers[index];
    }
  } eventListCtx;

  for (size_t i = 0; i < eventCount; ++i) {
    const auto &ev = events[i];
    if (ev.type == InputEventType::NoteOn ||
        ev.type == InputEventType::NoteOff) {
      clap_event_note_t note;
      note.header.size = sizeof(note);
      note.header.time = 0;
      note.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
      note.header.type = (ev.type == InputEventType::NoteOn)
                             ? CLAP_EVENT_NOTE_ON
                             : CLAP_EVENT_NOTE_OFF;
      note.header.flags = 0;
      note.port_index = 0;
      note.channel = 0;
      note.key = ev.id;
      note.velocity = ev.value;
      note.note_id = -1;
      eventListCtx.notes.push_back(note);
    } else if (ev.type == InputEventType::ParameterChange) {
      clap_event_param_value_t param;
      param.header.size = sizeof(param);
      param.header.time = 0;
      param.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
      param.header.type = CLAP_EVENT_PARAM_VALUE;
      param.header.flags = 0;
      param.param_id = ev.id;
      param.cookie = nullptr;
      param.note_id = -1;
      param.port_index = -1;
      param.channel = -1;
      param.key = -1;
      param.value = ev.value;
      eventListCtx.params.push_back(param);
    }
  }

  for (const auto &n : eventListCtx.notes)
    eventListCtx.headers.push_back(&n.header);
  for (const auto &p : eventListCtx.params)
    eventListCtx.headers.push_back(&p.header);

  clap_input_events_t in_events;
  in_events.ctx = &eventListCtx;
  in_events.size = InternalEventList::size;
  in_events.get = InternalEventList::get;

  clap_output_events_t out_events;
  out_events.ctx = states.get();
  out_events.try_push = [](const clap_output_events_t *list,
                           const clap_event_header_t *event) -> bool {
    if (event->space_id == CLAP_CORE_EVENT_SPACE_ID &&
        event->type == CLAP_EVENT_PARAM_VALUE) {
      auto *v = reinterpret_cast<const clap_event_param_value_t *>(event);
      auto *self = static_cast<PluginBridge::PluginBridgeInternal *>(list->ctx);
      if (self->param_edit_callback) {
        self->param_edit_callback(v->param_id, v->value);
      }
    }
    return true;
  };

  clap_process_t process;
  process.steady_time = -1;
  process.frames_count = nframes;
  process.transport = nullptr;
  process.audio_inputs = nullptr;
  process.audio_inputs_count = 0;
  process.audio_outputs = &audio_out;
  process.audio_outputs_count = 1;
  process.in_events = &in_events;
  process.out_events = &out_events;

  states->plugin->process(states->plugin, &process);
}

void PluginBridge::subscribeParameterEdit(
    std::function<void(uint32_t, double)> fn) {
  states->param_edit_callback = fn;
}

void PluginBridge::unsubscribeParameterEdit() {
  states->param_edit_callback = nullptr;
}

} // namespace clap_dev_host
