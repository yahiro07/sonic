#include "./my_synthesizer.h"
#include "./wrapper/clap_entry_wrapper.h"
#include <math.h>

PluginMeta meta = {
    .id = "com.my-company.my-plugin",
    .name = "MyPlugin",
    .vendor = "MyCompany",
    .url = "https://my-company.com",
    .manualUrl = "https://my-company.com/manual",
    .supportUrl = "https://my-company.com/support",
    .version = "1.0.0",
    .description = "Example CLAP plugin.",
};

extern "C" const clap_plugin_entry_t clap_entry =
    createClapPluginEntry(createSynthesizerInstance, meta);
