#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/plugprovider.h"
#include <filesystem>

using namespace Steinberg;
using namespace Steinberg::Vst;

void app1Entry() {
  printf("app1Entry\n");

  std::filesystem::path relPath =
      "../../templates/vst-simple/build/VST3/Debug/Project1.vst3";
  auto vstPath = std::filesystem::absolute(relPath).lexically_normal().string();

  printf("vstPath: %s\n", vstPath.c_str());
  Steinberg::Vst::HostApplication hostApp;

  std::string errorDescription;
  auto module = VST3::Hosting::Module::create(vstPath, errorDescription);
  if (!module) {
    printf("Failed to create module: %s\n", errorDescription.c_str());
    return;
  }
  auto factory = module->getFactory();
  VST3::Hosting::ClassInfo audioEffectClassInfo;
  for (auto &classInfo : factory.classInfos()) {
    printf("Class: %s %s\n", classInfo.name().c_str(),
           classInfo.category().c_str());
    if (classInfo.category() == kVstAudioEffectClass) {
      audioEffectClassInfo = classInfo;
      break;
    }
  }
  auto plugProvider = new PlugProvider(factory, audioEffectClassInfo, true);
  if (!plugProvider) {
    printf("Failed to create plug provider\n");
    return;
  }
  auto component = plugProvider->getComponent();
  if (!component) {
    printf("Failed to create vst plug\n");
    return;
  }

  auto audioProcessor = FUnknownPtr<IAudioProcessor>(component);
  if (!audioProcessor) {
    printf("Failed to query audio processor\n");
    return;
  }

  printf("loaded\n");
}