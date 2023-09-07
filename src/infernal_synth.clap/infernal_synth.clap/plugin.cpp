#include <infernal_synth/synth.hpp>
#include <infernal_synth/plugin.hpp>
#include <plugin_base.clap/plugin.hpp>
#include <plugin_base.clap/support.hpp>
#include <clap/clap.h>
#include <cstring>

using namespace infernal_synth;
using namespace plugin_base::clap;

#if INF_IS_FX
#define PLUG_FEATURE CLAP_PLUGIN_FEATURE_AUDIO_EFFECT
#else
#define PLUG_FEATURE CLAP_PLUGIN_FEATURE_INSTRUMENT
#endif

static char const*
features[] = { PLUG_FEATURE, CLAP_PLUGIN_FEATURE_STEREO, nullptr };

static clap_plugin_descriptor_t const descriptor =
{
  .clap_version = CLAP_VERSION_INIT,
  .id = INF_SYNTH_REVERSE_URI INF_SYNTH_ID,
  .name = INF_SYNTH_FULL_NAME,
  .vendor = INF_SYNTH_VENDOR_NAME,
  .url = INF_SYNTH_VENDOR_URL,
  .manual_url = INF_SYNTH_VENDOR_URL,
  .support_url = INF_SYNTH_VENDOR_URL,
  .version = INF_SYNTH_VERSION_TEXT,
  .description = INF_SYNTH_FULL_NAME,
  .features = features
};

extern "C" CLAP_EXPORT 
clap_plugin_entry_t const entry = 
{
  .clap_version = CLAP_VERSION_INIT,
  .init = [](char const*) { return true; },
  .deinit = [](){},
  .get_factory = get_plugin_factory<&descriptor, synth_topo>
};
