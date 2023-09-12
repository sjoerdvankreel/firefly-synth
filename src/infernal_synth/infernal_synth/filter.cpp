#include <plugin_base/dsp.hpp>
#include <plugin_base/topo.hpp>
#include <plugin_base/support.hpp>
#include <plugin_base/engine.hpp>
#include <infernal_synth/synth.hpp>
#include <cmath>

using namespace plugin_base;

namespace infernal_synth {

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
enum filter_param { filter_param_on, filter_param_freq, filter_param_out_gain };

module_group_topo
filter_topo()
{
  module_group_topo result(make_module_group("{4901E1B1-BFD6-4C85-83C4-699DC27C6BC4}", "Filter", 1, module_scope::voice, module_output::none));
  result.param_groups.emplace_back(param_group_topo(filter_group_main, "Main"));
  result.engine_factory = [](int sample_rate, int max_frame_count) -> std::unique_ptr<module_engine> { return std::make_unique<filter_engine>(); };
  result.params.emplace_back(param_toggle("{960E70F9-AB6E-4A9A-A6A7-B902B4223AF2}", "On", filter_group_main, param_text::both, false));
  result.params.emplace_back(param_log("{02D1D13E-7B78-4702-BB49-22B4E3AE1B1F}", "Freq", filter_group_main, param_display::knob, param_text::both, param_rate::accurate, 20, 20000, 1000, 1000, "Hz"));
  result.params.emplace_back(param_percentage("{6AB939E0-62D0-4BA3-8692-7FD7B740ED74}", "Out Gain", filter_group_main, param_display::knob, param_text::both, param_rate::block, true, 0, 1, 0));
  return result;
}

void
filter_engine::process(
  plugin_topo const& topo, plugin_block const& plugin, module_block& module)
{
  float max_out = 0.0f;
  auto const& osc_audio = plugin.module_audio[module_type::module_type_osc];
  for(int o = 0; o < topo.module_groups[module_type_osc].module_count; o++)
    for(int c = 0; c < 2; c++)
      for(int f = 0; f < plugin.host->frame_count; f++)
        plugin.host->audio_output[c][f] += osc_audio[o][c][f];
  int on = module.block_automation[filter_param_on].step();
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
      float filtered = plugin.host->audio_output[c][f] * a + _in[c] * a + _out[c] * b;
      _in[c] = plugin.host->audio_output[c][f];
      _out[c] = filtered;
      plugin.host->audio_output[c][f] = filtered;
      max_out = std::max(max_out, filtered);
    }
  }

  auto const& param = topo.module_groups[module_type_filter].params[filter_param_out_gain];
  module.output_values[filter_param_out_gain] = param.raw_to_plain(std::abs(max_out));
}

}
