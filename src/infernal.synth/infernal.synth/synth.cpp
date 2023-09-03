#include <infernal.synth/synth.hpp>
#include <infernal.base/engine.hpp>

using namespace infernal::base;

namespace infernal::synth {

class synth_mixdown:
public mixdown_engine
{
public:
  void process(plugin_topo const& topo, plugin_block const& plugin, float* const* mixdown) override;
};

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

void 
synth_mixdown::process(plugin_topo const& topo, plugin_block const& plugin, float* const* mixdown)
{
  auto const& filter_output = plugin.module_audio[module_type_filter][0];
  for(int c = 0; c < 2; c++)
    std::copy(filter_output[c].begin(), filter_output[c].begin() + plugin.host->frame_count, mixdown[c]);
}

}