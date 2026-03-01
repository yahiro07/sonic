// #include "public.sdk/source/vst/hosting/module.h"
// #include "public.sdk/source/vst/hosting/plugprovider.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
// #include "public.sdk/source/vst/hosting/eventlist.h"
// #include "public.sdk/source/vst/hosting/parameterchanges.h"#include <stdio.h>
#include "pluginterfaces/base/funknown.h"
#include "public.sdk/source/vst/hosting/module.h"
#include <filesystem>

using namespace Steinberg;

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
  printf("loaded\n");
}