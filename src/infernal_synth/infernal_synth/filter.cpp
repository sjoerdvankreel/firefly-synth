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
enum { param_on, param_freq, param_osc };

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
filter_topo(int section, plugin_base::gui_position const& pos, int osc_slot_count)
{
  module_topo result(make_module(
    make_topo_info("{4901E1B1-BFD6-4C85-83C4-699DC27C6BC4}", "Filter", module_filter, 3), 
    make_module_dsp(module_stage::voice, module_output::audio, 1, 0),
    make_module_gui(section, pos, gui_layout::tabbed, { 1, 1 })));

  result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag("{D32DC4C1-D0DD-462B-9AA9-A3B298F6F72F}", "Main"),
    make_param_section_gui({ 0, 0 }, { { 1 }, { gui_dimension::auto_size, 1, 3 } })));

  result.params.emplace_back(make_param(
    make_topo_info("{960E70F9-AB6E-4A9A-A6A7-B902B4223AF2}", "On", param_on, 1),
    make_param_dsp_block(param_automate::automate), make_domain_toggle(false),
    make_param_gui_single(section_main, gui_edit_type::toggle, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));

  result.params.emplace_back(make_param(
    make_topo_info("{02D1D13E-7B78-4702-BB49-22B4E3AE1B1F}", "Freq", param_freq, 1),
    make_param_dsp_accurate(param_automate::modulate), make_domain_log(20, 20000, 1000, 1000, 0, "Hz"),
    make_param_gui_single(section_main, gui_edit_type::knob, { 0, 1 }, 
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));

  result.params.emplace_back(make_param(
    make_topo_info("{B377EBB2-73E2-46F4-A2D6-867693ED9ACE}", "Osc", param_osc, osc_slot_count),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage(0, 1, 0.5, 0, true),
    make_param_gui(section_main, gui_edit_type::knob, gui_layout::horizontal, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));

  result.engine_factory = [](auto const&, int, int) ->
    std::unique_ptr<module_engine> { return std::make_unique<filter_engine>(); };

  return result;
}

void
filter_engine::process(plugin_block& block)
{
  if (block.state.own_block_automation[param_on][0].step() == 0) return;
  void* cv_matrix_context = block.voice->all_context[module_cv_matrix][0];
  auto const& modulation = static_cast<cv_matrix_output const*>(cv_matrix_context)->modulation;

  auto const& osc_audio = block.voice->all_audio[module_osc];
  for(int o = 0; o < block.plugin.modules[module_osc].info.slot_count; o++)
  {
    auto const& osc = *modulation[module_filter][block.module_slot][param_osc][o];
    for(int c = 0; c < 2; c++)
      for(int f = block.start_frame; f < block.end_frame; f++)
        block.state.own_audio[0][c][f] += osc_audio[o][0][c][f] * osc[f];
  }

  float w = 2 * block.sample_rate;
  auto const& freq = *modulation[module_filter][block.module_slot][param_freq][0];
  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    float angle = block.normalized_to_raw(module_filter, param_freq, freq[f]) * 2 * pi32;
    float norm = 1 / (angle + w);
    float a = angle * norm;
    float b = (w - angle) * norm;
    for (int c = 0; c < 2; c++)
    {
      float filtered = block.state.own_audio[0][c][f] * a + _in[c] * a + _out[c] * b;
      _in[c] = block.state.own_audio[0][c][f];
      _out[c] = filtered;
      block.state.own_audio[0][c][f] = filtered;
      // mixdown to voice
      block.voice->result[c][f] += filtered;
    }
  }
}

}
