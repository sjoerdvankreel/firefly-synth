#include <plugin_base/dsp.hpp>
#include <plugin_base/topo.hpp>
#include <plugin_base/support.hpp>
#include <plugin_base/engine.hpp>
#include <infernal_synth/synth.hpp>
#include <cmath>
#include <algorithm>

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

enum filter_section { filter_section_main };
enum filter_param { filter_param_on, filter_param_freq, filter_param_osc_gain, filter_param_out_gain };

module_topo
filter_topo()
{
  module_topo result(make_module("{4901E1B1-BFD6-4C85-83C4-699DC27C6BC4}", "Filter", 1, module_scope::voice, module_output::none));
  result.sections.emplace_back(section_topo(filter_section_main, "Main"));
  result.engine_factory = [](int sample_rate, int max_frame_count) -> std::unique_ptr<module_engine> { return std::make_unique<filter_engine>(); };
  result.params.emplace_back(param_toggle("{960E70F9-AB6E-4A9A-A6A7-B902B4223AF2}", "On", 1, filter_section_main, param_dir::input, param_label::both, false));
  result.params.emplace_back(param_log("{02D1D13E-7B78-4702-BB49-22B4E3AE1B1F}", "Freq", 1, filter_section_main, param_dir::input, param_edit::knob, param_label::both, param_rate::accurate, 20, 20000, 1000, 1000, "Hz"));
  result.params.emplace_back(param_pct("{B377EBB2-73E2-46F4-A2D6-867693ED9ACE}", "Osc Gain", 2, filter_section_main, param_dir::input, param_edit::vslider, param_label::both, param_rate::accurate, true, 0, 1, 0.5));
  result.params.emplace_back(param_pct("{6AB939E0-62D0-4BA3-8692-7FD7B740ED74}", "Out Gain", 1, filter_section_main, param_dir::output, param_edit::text, param_label::both, param_rate::block, true, 0, 1, 0));
  return result;
}

void
filter_engine::process(
  plugin_topo const& topo, plugin_block const& plugin, module_block& module)
{
  float max_out = 0.0f;
  auto const& osc_audio = plugin.out.audio[module_type::module_type_osc];
  auto const& osc_gain_curves = module.in.accurate()[filter_param_osc_gain];
  for(int o = 0; o < topo.modules[module_type_osc].slot_count; o++)
    for(int c = 0; c < 2; c++)
      for(int f = 0; f < plugin.host->frame_count; f++)
        plugin.host->audio_out[c][f] += osc_audio[o][c][f] * osc_gain_curves[o][f];
  int on = module.in.block()[filter_param_on][0].step();
  if (!on) return;

  float w = 2 * plugin.sample_rate;
  auto const& freq_curve = module.in.accurate()[filter_param_freq][0];
  for (int f = 0; f < plugin.host->frame_count; f++)
  {
    float angle = freq_curve[f] * 2 * pi32;
    float norm = 1 / (angle + w);
    float a = angle * norm;
    float b = (w - angle) * norm;
    for (int c = 0; c < 2; c++)
    {
      float filtered = plugin.host->audio_out[c][f] * a + _in[c] * a + _out[c] * b;
      _in[c] = plugin.host->audio_out[c][f];
      _out[c] = filtered;
      plugin.host->audio_out[c][f] = filtered;
      max_out = std::max(max_out, filtered);
    }
  }

  auto const& param = topo.modules[module_type_filter].params[filter_param_out_gain];
  module.out.params()[filter_param_out_gain][0] = param.raw_to_plain(std::clamp(std::abs(max_out), 0.0f, 1.0f));
}

}
