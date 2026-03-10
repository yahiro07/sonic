#pragma once

#include "../api/synthesizer-base.h"
#include "../core/parameter-builder-impl.h"
#include "../core/parameter-item.h"
#include "../domain/interfaces.h"
#include <cstdint>
#include <functional>

namespace sonic {

class IPlatformParameterIo {
public:
  virtual ~IPlatformParameterIo() = default;
  virtual void registerParameters(std::vector<ParameterItem> &params) = 0;
  virtual double getParameter(uint32_t id) = 0;
  virtual void setParameter(uint32_t id, double value) = 0;

  virtual void
  setParameterChangeCallback(std::function<void(uint32_t, double)> fn) = 0;
};

class PluginDomain {
private:
  SynthesizerBase &synth;
  IPlatformParameterIo &platformParameterIo;

public:
  PluginDomain(SynthesizerBase &synth,
               IPlatformParameterIo &platformParameterIo)
      : synth(synth), platformParameterIo(platformParameterIo) {}

  void initialize() {
    ParameterBuilderImpl builder;
    synth.setupParameters(builder);
    auto parameterItems = builder.getItems();
    platformParameterIo.registerParameters(parameterItems);
    for (auto &item : parameterItems) {
      synth.setParameter(item.id, item.defaultValue);
    }
    platformParameterIo.setParameterChangeCallback(
        [this](uint32_t id, double value) { synth.setParameter(id, value); });
  }

  void prepareProcessing(double sampleRate, uint32_t maxFrameCount) {
    synth.prepareProcessing(sampleRate, maxFrameCount);
  }

  void processAudio(float *bufferL, float *bufferR, uint32_t frames) {
    synth.processAudio(bufferL, bufferR, frames);
  }

  void setParameter(uint32_t id, double value) {
    synth.setParameter(id, value);
  }

  void noteOn(int noteNumber, double velocity) {
    synth.noteOn(noteNumber, velocity);
  }

  void noteOff(int noteNumber) { synth.noteOff(noteNumber); }

  void getDesiredEditorSize(uint32_t &width, uint32_t &height) {
    synth.getDesiredEditorSize(width, height);
  }
};
} // namespace sonic