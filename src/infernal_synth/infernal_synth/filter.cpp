#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>

#include <infernal_synth/synth.hpp>

#include <cmath>
#include <algorithm>

using namespace plugin_base;

namespace infernal_synth {

enum { section_main };
enum { param_on, param_freq, param_osc_gain };

std::vector<list_item>
cv_matrix_target_filter_params(module_topo const& filter_topo)
{
  std::vector<list_item> result;
  result.emplace_back(filter_topo.params[param_freq]);
  result.emplace_back(filter_topo.params[param_osc_gain]);
  return result;
}

class filter_engine: 
public module_engine {  
  float _in[2];
  float _out[2];

public:
  filter_engine() { initialize(); }
  INF_PREVENT_ACCIDENTAL_COPY(filter_engine);
  void process(plugin_block& block) override;
  void initialize() override { _in[0] = _in[1] = _out[0] = _out[1] = 0; }
};

module_topo
filter_topo(int osc_slot_count)
{
  module_topo result(make_module(
    make_topo_info("{4901E1B1-BFD6-4C85-83C4-699DC27C6BC4}", "Voice Filter", module_filter, 1), 
    make_module_dsp(module_stage::voice, module_output::none, 0),
    make_module_gui(gui_layout::single, { 3, 0 }, { 1, 1 })));
  result.engine_factory = [](int, int, int) ->
    std::unique_ptr<module_engine> { return std::make_unique<filter_engine>(); };
  result.sections.emplace_back(make_section(section_main,
    make_topo_tag("{D32DC4C1-D0DD-462B-9AA9-A3B298F6F72F}", "Main"),
    make_section_gui({ 0, 0 }, { { 1 }, { 1, 1, 2} })));
  result.params.emplace_back(make_param(
    make_topo_info("{960E70F9-AB6E-4A9A-A6A7-B902B4223AF2}", "On", param_on, 1),
    make_param_dsp_block(), make_domain_toggle(false),
    make_param_gui_single(section_main, gui_edit_type::toggle, { 0, 0 }, 
      make_label_default(gui_label_contents::name))));  
  result.params.emplace_back(make_param(
    make_topo_info("{02D1D13E-7B78-4702-BB49-22B4E3AE1B1F}", "Freq", param_freq, 1),
    make_param_dsp_accurate(param_format::normalized), make_domain_log(20, 20000, 1000, 1000, 0, "Hz"),
    make_param_gui_single(section_main, gui_edit_type::hslider, { 0, 1 }, 
      make_label(gui_label_contents::name, gui_label_align::bottom, gui_label_justify::near))));
  result.params.emplace_back(make_param(
    make_topo_info("{B377EBB2-73E2-46F4-A2D6-867693ED9ACE}", "Osc Gain", param_osc_gain, osc_slot_count),
    make_param_dsp_accurate(param_format::plain), make_domain_percentage(0, 1, 0.5, 0, true),
    make_param_gui(section_main, gui_edit_type::hslider, gui_layout::horizontal, { 0, 2 }, 
      make_label(gui_label_contents::name, gui_label_align::bottom, gui_label_justify::near))));
  return result;
}

void
filter_engine::process(plugin_block& block)
{
  auto const& osc_audio = block.voice->audio_in[module_osc];
  auto const& osc_gain = block.state.accurate_automation[param_osc_gain];
  for(int o = 0; o < block.plugin.modules[module_osc].info.slot_count; o++)
    for(int c = 0; c < 2; c++)
      for(int f = block.start_frame; f < block.end_frame; f++)
        block.voice->result[c][f] += osc_audio[o][0][c][f] * osc_gain[o][f];
  if(block.state.block_automation[param_on][0].step() == 0) return;

  float w = 2 * block.sample_rate;
  auto const& env = block.voice->cv_in[module_env][1][0];
  auto const& freq = block.state.accurate_automation[param_freq][0];
  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    float angle = block.normalized_to_raw(module_filter, param_freq, freq[f] * env[f]) * 2 * pi32;
    float norm = 1 / (angle + w);
    float a = angle * norm;
    float b = (w - angle) * norm;
    for (int c = 0; c < 2; c++)
    {
      float filtered = block.voice->result[c][f] * a + _in[c] * a + _out[c] * b;
      _in[c] = block.voice->result[c][f];
      _out[c] = filtered;
      block.voice->result[c][f] = filtered;
    }
  }
}

}
