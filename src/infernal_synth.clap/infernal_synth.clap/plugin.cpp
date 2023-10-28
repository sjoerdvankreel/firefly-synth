#include <infernal_synth/synth.hpp>
#include <infernal_synth/plugin.hpp>

#include <plugin_base/gui/gui.hpp>
#include <plugin_base/gui/utility.hpp>
#include <plugin_base.clap/inf_plugin.hpp>

#include <juce_gui_basics/juce_gui_basics.h>
#include <clap/clap.h>
#include <cstring>

using namespace plugin_base;
using namespace infernal_synth;
using namespace plugin_base::clap;

#if INF_IS_FX
#define PLUG_FEATURE CLAP_PLUGIN_FEATURE_AUDIO_EFFECT
#else
#define PLUG_FEATURE CLAP_PLUGIN_FEATURE_INSTRUMENT
#endif

static std::unique_ptr<plugin_topo> _topo = {};
static std::unique_ptr<plugin_desc> _desc = {};

static char const*
features[] = { PLUG_FEATURE, CLAP_PLUGIN_FEATURE_STEREO, nullptr };

static void CLAP_ABI
deinit()
{
  juce::shutdownJuce_GUI();
  _desc.reset();
  _topo.reset();
}

static bool CLAP_ABI
init(char const*)
{
  _topo = synth_topo();
  _desc = std::make_unique<plugin_desc>(_topo.get());
  juce::initialiseJuce_GUI();
  return true;
}

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

static void const* 
get_plugin_factory(char const* factory_id)
{
  static clap_plugin_factory result;
  if (strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID)) return nullptr;
  result.get_plugin_count = [](clap_plugin_factory const*) { return 1u; };
  result.get_plugin_descriptor = [](clap_plugin_factory const*, std::uint32_t) { return &descriptor; };
  result.create_plugin = [](clap_plugin_factory const*, clap_host const* host, char const*)
  { return (new inf_plugin(&descriptor, host, _desc.get()))->clapPlugin(); };
  return &result;
}

extern "C" CLAP_EXPORT 
clap_plugin_entry_t const clap_entry =
{
  .clap_version = CLAP_VERSION_INIT,
  .init = init,
  .deinit = deinit,
  .get_factory = get_plugin_factory
};
