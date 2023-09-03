#include <infernal.synth/synth.hpp>

using namespace infernal::base;

namespace infernal::synth {

plugin_topo
synth_topo()
{
  plugin_topo result;
  result.polyphony = 32;
  result.type = plugin_type::synth;
  result.module_groups.emplace_back(osc_topo());
  result.module_groups.emplace_back(filter_topo());
  result.mixdown_factory = [](int sample_rate, int max_frame_count) -> std::unique_ptr<mixdown_engine> { return std::make_unique<synth_mixdown>(); };
  return result;
}

}