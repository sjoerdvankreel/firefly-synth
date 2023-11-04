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
enum { type_off, type_lpf, type_hpf };
enum { param_type, param_freq, param_res, param_osc };

static std::vector<list_item>
type_items()
{
  std::vector<list_item> result;
  result.emplace_back("{F37A19CE-166A-45BF-9F75-237324221C39}", "Off");
  result.emplace_back("{8B7E5C75-C3F6-4F53-900C-0A75703F5570}", "LPF");
  result.emplace_back("{673C872A-F740-431C-8CD3-F577CE984C2D}", "HPF");
  return result;
}

class fx_engine: 
public module_engine {  
  double _ic1eq[2];
  double _ic2eq[2];

  void process_filter(plugin_block& block);

public:
  fx_engine() { initialize(); }
  INF_PREVENT_ACCIDENTAL_COPY(fx_engine);
  void process(plugin_block& block) override;
  void initialize() override { _ic1eq[0] = _ic1eq[1] = _ic2eq[0] = _ic2eq[1] = 0; }
};

module_topo
fx_topo(
  int section, plugin_base::gui_colors const& colors,
  plugin_base::gui_position const& pos, int osc_slot_count)
{
  module_topo result(make_module(
    make_topo_info("{4901E1B1-BFD6-4C85-83C4-699DC27C6BC4}", "FX", module_fx, 3), 
    make_module_dsp(module_stage::voice, module_output::audio, 1, 0),
    make_module_gui(section, colors, pos, { 1, 1 })));

  result.engine_factory = [](auto const&, int, int) ->
    std::unique_ptr<module_engine> { return std::make_unique<fx_engine>(); };

  result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag("{D32DC4C1-D0DD-462B-9AA9-A3B298F6F72F}", "Main"),
    make_param_section_gui({ 0, 0 }, { { 1 }, { gui_dimension::auto_size, 1, 1, 3 } })));

  result.params.emplace_back(make_param(
    make_topo_info("{960E70F9-AB6E-4A9A-A6A7-B902B4223AF2}", "On", param_type, 1),
    make_param_dsp_block(param_automate::automate), make_domain_item(type_items(), ""),
    make_param_gui_single(section_main, gui_edit_type::autofit_list, { 0, 0 }, make_label_none())));

  auto& freq = result.params.emplace_back(make_param(
    make_topo_info("{02D1D13E-7B78-4702-BB49-22B4E3AE1B1F}", "Freq", param_freq, 1),
    make_param_dsp_accurate(param_automate::both), make_domain_log(20, 20000, 1000, 1000, 0, "Hz"),
    make_param_gui_single(section_main, gui_edit_type::knob, { 0, 1 }, 
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  freq.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_lpf || vs[0] == type_hpf; });

  auto& res = result.params.emplace_back(make_param(
    make_topo_info("{71A30AC8-5291-467A-9662-BE09F0278A3B}", "Res", param_res, 1),
    make_param_dsp_accurate(param_automate::both), make_domain_percentage(0, 1, 0, 0, true),
    make_param_gui_single(section_main, gui_edit_type::knob, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  res.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_lpf || vs[0] == type_hpf; });

  result.params.emplace_back(make_param(
    make_topo_info("{B377EBB2-73E2-46F4-A2D6-867693ED9ACE}", "Osc", param_osc, osc_slot_count),
    make_param_dsp_accurate(param_automate::both), make_domain_percentage(0, 1, 0.5, 0, true),
    make_param_gui(section_main, gui_edit_type::knob, param_layout::horizontal, { 0, 3 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));

  return result;
}

void
fx_engine::process_filter(plugin_block& block)
{
  int type = block.state.own_block_automation[param_type][0].step();
  auto const& modulation = get_cv_matrix_mixdown(block);
  auto const& res_curve = *modulation[module_fx][block.module_slot][param_res][0];
  auto const& freq_curve = *modulation[module_fx][block.module_slot][param_freq][0];

  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    float freq = block.normalized_to_raw(module_fx, param_freq, freq_curve[f]);
    double k = 2 - 2 * res_curve[f];
    double g = std::tan(pi32 * freq / block.sample_rate);
    double a1 = 1 / (1 + g * (g + k));
    double a2 = g * a1;
    double a3 = g * a2;

    for (int c = 0; c < 2; c++)
    {
      double v0 = block.state.own_audio[0][c][f];
      double v3 = v0 - _ic2eq[c];
      double v1 = a1 * _ic1eq[c] + a2 * v3;
      double v2 = _ic2eq[c] + a2 * _ic1eq[c] + a3 * v3;
      _ic1eq[c] = 2 * v1 - _ic1eq[c];
      _ic2eq[c] = 2 * v2 - _ic2eq[c];
      float out = type == type_lpf? v2: v0 - k * v1 - v2;
      block.state.own_audio[0][c][f] = out;
    }
  }
}

void
fx_engine::process(plugin_block& block)
{
  int type = block.state.own_block_automation[param_type][0].step();
  auto const& modulation = get_cv_matrix_mixdown(block);
  auto const& osc_audio = block.voice->all_audio[module_osc];
  for(int o = 0; o < block.plugin.modules[module_osc].info.slot_count; o++)
  {
    auto const& osc = *modulation[module_fx][block.module_slot][param_osc][o];
    for(int c = 0; c < 2; c++)
      for(int f = block.start_frame; f < block.end_frame; f++)
        block.state.own_audio[0][c][f] += osc_audio[o][0][c][f] * osc[f];
  }

  if(type == type_lpf || type == type_hpf)
    process_filter(block);  
  for(int c = 0; c < 2; c++)
    block.state.own_audio[0][c].add_to(block.start_frame, block.end_frame, block.voice->result[c]);
}

}
