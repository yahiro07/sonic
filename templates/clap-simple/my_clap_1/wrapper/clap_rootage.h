#pragma once

#include "clap/clap.h"
#include "clap/plugin.h"
#include <functional>

class PlugBasis {
public:
  clap_plugin_t clapPlugin;

  virtual ~PlugBasis() = default;
  virtual void setSampleRate(double sampleRate) = 0;
  virtual clap_process_status process(const clap_process_t *processData) = 0;

  virtual uint32_t getParameterCount() const = 0;
  virtual void getParameterInfo(uint32_t index,
                                clap_param_info_t *info) const = 0;
  virtual double getParameterValue(clap_id id) const = 0;
  virtual void setParameterValue(clap_id id, double value) = 0;

  virtual bool guiCreate() = 0;
  virtual void guiDestroy() = 0;
  virtual bool guiSetParent(const clap_window_t *window) = 0;
  virtual bool guiSetSize(uint32_t width, uint32_t height) = 0;
  virtual bool guiShow() = 0;
  virtual bool guiHide() = 0;
};

void clapRootage_setPluginBasisInstantiateFn(
    const std::function<PlugBasis *()> fn);
clap_plugin_descriptor_t &clapRootage_getPluginDescriptor();
const clap_plugin_entry_t &clapRootage_getClapPluginEntry();
