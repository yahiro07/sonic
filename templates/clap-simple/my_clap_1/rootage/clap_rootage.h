#pragma once

#include "clap/clap.h"
#include "clap/host.h"
#include "clap/plugin.h"
#include <functional>

class PlugBasis {
public:
  clap_plugin_t plugin;
  const clap_host_t *host = nullptr;
  const clap_host_params_t *hostParams = nullptr;

  virtual ~PlugBasis() = default;

  virtual void initialize() = 0; // called after host and hostParams are set

  virtual void setSampleRate(double sampleRate) = 0;
  virtual clap_process_status process(const clap_process_t *processData) = 0;
  virtual void flushParameters(const clap_input_events_t *inputEvents,
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

  virtual void onMainThread() {}
};

void clapRootage_setPluginBasisInstantiateFn(
    const std::function<PlugBasis *()> fn);
clap_plugin_descriptor_t &clapRootage_getPluginDescriptor();
const clap_plugin_entry_t &clapRootage_getClapPluginEntry();
