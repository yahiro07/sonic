#pragma once

#include <clap/clap.h>
#include <cstdint>

class SynthesizerBase {
public:
  virtual ~SynthesizerBase() = default;
  virtual void setSampleRate(double sampleRate) = 0;
  virtual void processAudio(float *bufferL, float *bufferR,
                            uint32_t frames) = 0;
  virtual void noteOn(int noteNumber, double velocity) = 0;
  virtual void noteOff(int noteNumber) = 0;

  // these are temporal interface
  virtual uint32_t getParameterCount() const = 0;
  virtual void getParameterInfo(uint32_t index,
                                clap_param_info_t *info) const = 0;
  virtual double getParameterValue(clap_id id) const = 0;
  virtual void setParameterValue(clap_id id, double value) = 0;
};

typedef SynthesizerBase *(*synthesizerInitializerFn)();

struct PluginMeta {
  const char *id;
  const char *name;
  const char *vendor;
  const char *url;
  const char *manualUrl;
  const char *supportUrl;
  const char *version;
  const char *description;
};

const clap_plugin_entry_t &
createClapPluginEntry(synthesizerInitializerFn synthInitializer,
                      const PluginMeta &meta);