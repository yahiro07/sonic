#pragma once

#include <clap/clap.h>

class EntryController {
public:
  clap_plugin_t plugin;
  const clap_host_t *host = nullptr;
  const clap_host_params_t *hostParams = nullptr;
  const clap_host_timer_support_t *hostTimerSupport = nullptr;

  virtual ~EntryController() = default;

  virtual void initialize() = 0; // called after host and hostParams are set

  virtual void prepareProcessing(double sampleRate, uint32_t maxFrameCount) = 0;
  virtual clap_process_status process(const clap_process_t *processData) = 0;
  virtual void flush(const clap_input_events_t *inputEvents,
                     const clap_output_events_t *outputEvents) = 0;

  virtual uint32_t getParameterCount() const = 0;
  virtual void getParameterInfo(uint32_t index,
                                clap_param_info_t *info) const = 0;
  virtual double getParameterValue(clap_id id) const = 0;

  virtual bool guiCreate() = 0;
  virtual void guiDestroy() = 0;
  virtual bool guiSetParent(const clap_window_t *window) = 0;
  virtual bool guiSetSize(uint32_t width, uint32_t height) = 0;
  virtual bool guiShow() = 0;
  virtual bool guiHide() = 0;

  virtual void onTimer(clap_id timerId) {}
  virtual void onMainThread() {}

  static EntryController *create(void *synthInstance);
};