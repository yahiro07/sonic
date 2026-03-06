#pragma once

#include "./plug_basis.h"
#include "clap/clap.h"
#include <functional>

void clapRootage_setPluginBasisInstantiateFn(
    const std::function<PlugBasis *()> fn);
clap_plugin_descriptor_t &clapRootage_getPluginDescriptor();
const clap_plugin_entry_t &clapRootage_getClapPluginEntry();
