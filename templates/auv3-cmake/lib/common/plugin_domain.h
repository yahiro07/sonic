#pragma once

#include "interfaces.h"
#include "parameter_builder_impl.h"
#include "parameter_item.h"
#include "synthesizer_base.h"
#include <cstdint>
#include <functional>

namespace sonic_common {

class PluginDomain : public IPluginDomain {
private:
  SynthesizerBase &synth;
  IPlatformParameterIo &platformParameterIo;

public:
  PluginDomain(SynthesizerBase &synth,
               IPlatformParameterIo &platformParameterIo)
      : synth(synth), platformParameterIo(platformParameterIo) {}

  void initialize() override {
    sonic_common::ParameterBuilderImpl builder;
    synth.setupParameters(builder);
    auto parameterItems = builder.getItems();
    platformParameterIo.registerParameters(parameterItems);
    for (auto &item : parameterItems) {
      synth.setParameter(item.address, item.defaultValue);
    }
    platformParameterIo.setParameterChangeCallback(
        [this](uint64_t address, double value) {
          synth.setParameter(address, value);
        });
  }

  void prepareProcessing(double sampleRate, uint32_t maxFrameCount) override {
    synth.prepareProcessing(sampleRate, maxFrameCount);
  }

  void processAudio(float *bufferL, float *bufferR, uint32_t frames) override {
    synth.processAudio(bufferL, bufferR, frames);
  }

  void setParameter(uint64_t address, double value) override {
    synth.setParameter(address, value);
  }

  void noteOn(int noteNumber, double velocity) override {
    synth.noteOn(noteNumber, velocity);
  }

  void noteOff(int noteNumber) override { synth.noteOff(noteNumber); }

  void getDesiredEditorSize(uint32_t &width, uint32_t &height) override {
    synth.getDesiredEditorSize(width, height);
  }
};
} // namespace sonic_common