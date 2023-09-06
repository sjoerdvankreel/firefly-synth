#include <plugin_base/dsp.hpp>
#include <plugin_base/topo.hpp>
#include <plugin_base/support.hpp>
#include <plugin_base/engine.hpp>
#include <infernal_synth/synth.hpp>
#include <cmath>

using namespace infernal::base;

namespace infernal::synth {

class filter_engine: 
public module_engine {  
  float _in[2] = { 0, 0 };
  float _out[2] = { 0, 0 };
public:
  void process(
    plugin_topo const& topo, 
    plugin_block const& plugin, 
    module_block& module) override;
};

enum filter_group { filter_group_main };
enum filter_param { filter_param_on, filter_param_freq };

module_group_topo
filter_topo()
{
  module_group_topo result(make_module_group("{4901E1B1-BFD6-4C85-83C4-699DC27C6BC4}", "Filter", 1, module_scope::voice, module_output::audio));
  result.param_groups.emplace_back(param_group_topo(filter_group_main, "Main"));
  result.engine_factory = [](int sample_rate, int max_frame_count) -> std::unique_ptr<module_engine> { return std::make_unique<filter_engine>(); };
  result.params.emplace_back(param_toggle("{960E70F9-AB6E-4A9A-A6A7-B902B4223AF2}", "On", filter_group_main, false));
  result.params.emplace_back(param_log("{02D1D13E-7B78-4702-BB49-22B4E3AE1B1F}", "Freq", filter_group_main, param_display::knob, param_rate::accurate, 20, 20000, 1000, 1000, "Hz"));
  return result;
}

void
filter_engine::process(
  plugin_topo const& topo, plugin_block const& plugin, module_block& module)
{
  auto const& osc_audio = plugin.module_audio[module_type::module_type_osc];
  for(int o = 0; o < topo.module_groups[module_type_osc].module_count; o++)
    for(int c = 0; c < 2; c++)
      for(int f = 0; f < plugin.host->frame_count; f++)
        module.audio_output[c][f] += osc_audio[o][c][f];
  int on = module.block_automation[filter_param_on].step;
  if (!on) return;

  float w = 2 * plugin.sample_rate;
  auto const& freq_curve = module.accurate_automation[filter_param_freq];
  for (int f = 0; f < plugin.host->frame_count; f++)
  {
    float angle = freq_curve[f] * 2 * INF_PI;
    float norm = 1 / (angle + w);
    float a = angle * norm;
    float b = (w - angle) * norm;
    for (int c = 0; c < 2; c++)
    {
      float filtered = module.audio_output[c][f] * a + _in[c] * a + _out[c] * b;
      _in[c] = module.audio_output[c][f];
      _out[c] = filtered;
      module.audio_output[c][f] = filtered;
    }
  }
}

}
