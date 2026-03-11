#pragma once

#include "./entry-controller.h"
#include <functional>

void clapRootage_setEntryControllerInstantiateFn(
    const std::function<EntryController *()> fn);
clap_plugin_descriptor_t &clapRootage_getPluginDescriptor();
const clap_plugin_entry_t &clapRootage_getClapPluginEntry();
