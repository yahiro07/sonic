#pragma once

#include "parameter_builder_impl.h"
#include "parameter_item.h"
#include "synthesizer_base.h"
#include <cstdint>
#include <functional>

class IPlatformParameterIo {
public:
  virtual ~IPlatformParameterIo() = default;
  virtual void
  registerParameters(std::vector<sonic_common::ParameterItem> &params) = 0;
  virtual double getParameter(uint64_t address) = 0;
  virtual void setParameter(uint64_t address, double value) = 0;

  virtual void
  setParameterChangeCallback(std::function<void(uint64_t, double)> fn) = 0;
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

  void prepareProcessing(double sampleRate, uint32_t maxFrameCount) {
    synth.prepareProcessing(sampleRate, maxFrameCount);
  }

  void processAudio(float *bufferL, float *bufferR, uint32_t frames) {
    synth.processAudio(bufferL, bufferR, frames);
  }

  void setParameter(uint64_t address, double value) {
    synth.setParameter(address, value);
  }

  void noteOn(int noteNumber, double velocity) {
    synth.noteOn(noteNumber, velocity);
  }

  void noteOff(int noteNumber) { synth.noteOff(noteNumber); }

  void getDesiredEditorSize(uint32_t &width, uint32_t &height) {
    synth.getDesiredEditorSize(width, height);
  }
};