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
enum { type_off, type_lpf, type_hpf, type_delay };
enum { param_type, param_filter_freq, param_filter_res, param_delay_tempo, param_delay_gain, param_delay_feedback };

static std::vector<list_item>
type_items()
{
  std::vector<list_item> result;
  result.emplace_back("{F37A19CE-166A-45BF-9F75-237324221C39}", "Off");
  result.emplace_back("{8B7E5C75-C3F6-4F53-900C-0A75703F5570}", "LPF");
  result.emplace_back("{673C872A-F740-431C-8CD3-F577CE984C2D}", "HPF");
  result.emplace_back("{789D430C-9636-4FFF-8C75-11B839B9D80D}", "Delay");
  return result;
}

class fx_engine: 
public module_engine {  
  double _ic1eq[2];
  double _ic2eq[2];

  void process_delay(plugin_block& block);
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
    make_param_section_gui({ 0, 0 }, { { 1 }, { gui_dimension::auto_size, 1, 1, 1 } })));

  result.params.emplace_back(make_param(
    make_topo_info("{960E70F9-AB6E-4A9A-A6A7-B902B4223AF2}", "On", param_type, 1),
    make_param_dsp_block(param_automate::automate), make_domain_item(type_items(), ""),
    make_param_gui_single(section_main, gui_edit_type::autofit_list, { 0, 0 }, make_label_none())));

  auto& filter_freq = result.params.emplace_back(make_param(
    make_topo_info("{02D1D13E-7B78-4702-BB49-22B4E3AE1B1F}", "Freq", param_filter_freq, 1),
    make_param_dsp_accurate(param_automate::both), make_domain_log(20, 20000, 1000, 1000, 0, "Hz"),
    make_param_gui_single(section_main, gui_edit_type::knob, { 0, 1 }, 
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  filter_freq.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_delay; });
  filter_freq.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_lpf || vs[0] == type_hpf; });

  auto& filter_res = result.params.emplace_back(make_param(
    make_topo_info("{71A30AC8-5291-467A-9662-BE09F0278A3B}", "Res", param_filter_res, 1),
    make_param_dsp_accurate(param_automate::both), make_domain_percentage(0, 1, 0, 0, true),
    make_param_gui_single(section_main, gui_edit_type::knob, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  filter_res.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] != type_delay; });
  filter_res.gui.bindings.enabled.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_lpf || vs[0] == type_hpf; });

  auto& delay_tempo = result.params.emplace_back(make_param(
    make_topo_info("{C2E282BA-9E4F-4AE6-A055-8B5456780C66}", "Tempo", param_delay_tempo, 1),
    make_param_dsp_block(param_automate::automate), make_domain_timesig_default(),
    make_param_gui_single(section_main, gui_edit_type::list, { 0, 1 }, make_label_none())));
  delay_tempo.gui.submenu = make_timesig_submenu(delay_tempo.domain.timesigs);
  delay_tempo.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_delay; });

  auto& delay_gain = result.params.emplace_back(make_param(
    make_topo_info("{037E4A64-8F80-4E0A-88A0-EE1BB83C99C6}", "Gain", param_delay_gain, 1),
    make_param_dsp_accurate(param_automate::both), make_domain_percentage(0, 1, 0.5, 0, true),
    make_param_gui_single(section_main, gui_edit_type::knob, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  delay_gain.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_delay; });

  auto& delay_feedback = result.params.emplace_back(make_param(
    make_topo_info("{8FE78CA0-5752-4194-B4FE-E66017303687}", "Fbdk", param_delay_feedback, 1),
    make_param_dsp_accurate(param_automate::both), make_domain_percentage(0, 1, 0.5, 0, true),
    make_param_gui_single(section_main, gui_edit_type::knob, { 0, 3 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  delay_feedback.gui.bindings.visible.bind_params({ param_type }, [](auto const& vs) { return vs[0] == type_delay; });

  return result;
}

void
fx_engine::process(plugin_block& block)
{
  int type = block.state.own_block_automation[param_type][0].step();
  auto& mixer = get_audio_matrix_mixer(block);
  auto const& audio_in = mixer.mix(block, module_fx, block.module_slot);
  for (int c = 0; c < 2; c++)
    audio_in[c].copy_to(block.start_frame, block.end_frame, block.state.own_audio[0][c]);
  
  if (type == type_delay)
    process_delay(block);
  if (type == type_lpf || type == type_hpf)
    process_filter(block);
}

void
fx_engine::process_delay(plugin_block& block)
{
}
  
void
fx_engine::process_filter(plugin_block& block)
{
  int type = block.state.own_block_automation[param_type][0].step();
  auto const& modulation = get_cv_matrix_mixdown(block);
  auto const& res_curve = *modulation[module_fx][block.module_slot][param_filter_res][0];
  auto const& freq_curve = *modulation[module_fx][block.module_slot][param_filter_freq][0];

  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    float freq = block.normalized_to_raw(module_fx, param_filter_freq, freq_curve[f]);
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

}
