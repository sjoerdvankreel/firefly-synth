#include <infernal_synth/synth.hpp>
#include <infernal_synth/plugin.hpp>
#include <plugin_base/engine.hpp>

using namespace plugin_base;

namespace infernal_synth {

plugin_topo
synth_topo()
{
  plugin_topo result;
  result.polyphony = 32;
  result.id = INF_SYNTH_ID;
  result.type = plugin_type::synth;
  result.gui_default_width = 800;
  result.gui_aspect_ratio = 4.0f /3.0f;
  result.modules.emplace_back(osc_topo());
  result.modules.emplace_back(filter_topo());
  result.version_minor = INF_SYNTH_VERSION_MINOR;
  result.version_major = INF_SYNTH_VERSION_MAJOR;
  return result;
}

}