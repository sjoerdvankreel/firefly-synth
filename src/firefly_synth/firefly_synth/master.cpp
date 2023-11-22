#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>

#include <infernal_synth/synth.hpp>
#include <cmath>

using namespace plugin_base;

namespace infernal_synth {

enum { section_main };
enum { param_gain, param_bal };

class master_engine :
public module_engine {

public:
  void initialize() override { }
  void process(plugin_block& block) override;
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(master_engine);
};

module_topo
master_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos)
{
  module_topo result(make_module(
    make_topo_info("{3EEB56AB-FCBC-4C15-B6F3-536DB0D93E67}", "Master", module_master, 1),
    make_module_dsp(module_stage::output, module_output::none, 0, {}),
    make_module_gui(section, colors, pos, { 1, 1 })));

  result.engine_factory = [](auto const&, int, int) ->
    std::unique_ptr<master_engine> { return std::make_unique<master_engine>(); };

  result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag("{34BF24A3-696C-48F5-A49F-7CA445DEF38E}", "Main"),
    make_param_section_gui({ 0, 0 }, gui_dimension({ 1 }, { 1, 1 }))));

  result.params.emplace_back(make_param(
    make_topo_info("{2156DEE6-A147-4B93-AEF3-ABE69F53DBF9}", "Gain", param_gain, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(0, 1, 1, 0, true),
    make_param_gui_single(section_main, gui_edit_type::knob, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));

  result.params.emplace_back(make_param(
    make_topo_info("{7CCD4A32-FD84-402E-B099-BB94AAAD3C9E}", "Bal", param_bal, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(-1, 1, 0, 0, true),
    make_param_gui_single(section_main, gui_edit_type::knob, { 0, 1 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));

  return result;
}

void
master_engine::process(plugin_block& block)
{
  auto& mixer = get_audio_matrix_mixer(block, true);
  auto const& audio_in = mixer.mix(block, module_master, 0);
  auto const& modulation = get_cv_matrix_mixdown(block, true);
  auto const& bal_curve = *modulation[module_master][0][param_bal][0];
  auto const& gain_curve = *modulation[module_master][0][param_gain][0];
  for (int f = block.start_frame; f < block.end_frame; f++)
  {
    float bal = block.normalized_to_raw(module_master, param_bal, bal_curve[f]);
    for(int c = 0; c < 2; c++)
      block.out->host_audio[c][f] = audio_in[c][f] * gain_curve[f] * balance(c, bal);
  }
}

}
