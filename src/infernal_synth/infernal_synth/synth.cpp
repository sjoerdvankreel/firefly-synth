#include <infernal_synth/synth.hpp>
#include <plugin_base/engine.hpp>

using namespace plugin_base;

namespace infernal_synth {

plugin_topo
synth_topo()
{
  plugin_topo result;
  result.polyphony = 32;
  result.type = plugin_type::synth;
  result.module_groups.emplace_back(osc_topo());
  result.module_groups.emplace_back(filter_topo());
  return result;
}

}