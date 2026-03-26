#pragma once

#include "./entry_controller_interface.h"
#include "clap/clap.h"
#include <functional>

void clapRootage_setEntryControllerInstantiateFn(
    const std::function<IEntryController *()> fn);
clap_plugin_descriptor_t &clapRootage_getPluginDescriptor();
const clap_plugin_entry_t &clapRootage_getClapPluginEntry();
