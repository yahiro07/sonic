#pragma once
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/plugprovider.h"
#include <string>

namespace vst_dev_host {

class PluginBridge {
public:
  PluginBridge();
  ~PluginBridge();

  void loadPlugin(const std::string &path);
  void createEditor(void *ownerViewHandle);
  void closeEditor();
  void unloadPlugin();
  void prepareAudio(double sampleRate, int maxBlockSize);
  void processAudio(float *bufferL, float *bufferR, int nframes);

private:
  VST3::Hosting::Module::Ptr module;
  Steinberg::IPtr<Steinberg::Vst::PlugProvider> plugProvider;
  Steinberg::FUnknownPtr<Steinberg::Vst::IAudioProcessor> audioProcessor;
  Steinberg::FUnknownPtr<Steinberg::Vst::IEditController> editController;
  Steinberg::IPlugView *plugView = nullptr;

  double sampleRate = 44100.0;
  int maxBlockSize = 512;
};

} // namespace vst_dev_host