#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/dsp/graph_engine.hpp>
#include <plugin_base/shared/io_plugin.hpp>
#include <plugin_base/shared/tuning.hpp>

#include <firefly_synth/synth.hpp>

using namespace plugin_base;

namespace firefly_synth {

static int const max_auto_smoothing_ms = 50;
static int const max_other_smoothing_ms = 1000;

enum { section_main }; 
enum { param_midi_smooth, param_tempo_smooth, param_auto_smooth, param_count };

// we provide the buttons, everyone else needs to implement it
extern int const master_settings_param_auto_smooth = param_auto_smooth;
extern int const master_settings_param_midi_smooth = param_midi_smooth;
extern int const master_settings_param_tempo_smooth = param_tempo_smooth;

static graph_data
render_graph(plugin_state const& state, graph_engine* engine, int param, param_topo_mapping const& mapping)
{
  float value = state.get_plain_at(mapping).real();
  if (mapping.param_index == param_auto_smooth)
    value /= max_auto_smoothing_ms;
  else if (mapping.param_index == param_midi_smooth || mapping.param_index == param_tempo_smooth)
    value /= max_other_smoothing_ms;
  else
    return graph_data(graph_data_type::na, {});
  std::string partition = state.desc().params[param]->info.name;
  return graph_data(value, false, { partition });
}

module_topo
master_settings_topo(std::string const& vendor, std::string const& full_name, int section, gui_position const& pos)
{
  std::vector<int> row_distribution = { 1 };
  std::vector<int> column_distribution = { 1 };
  module_topo result(make_module( 
    make_topo_info_basic("{7F400614-E996-4B02-9B78-80E22F1C44A4}", "Master", module_master_settings, 1),
    make_module_dsp(module_stage::input, module_output::none, 0, {}),
      make_module_gui(section, pos, { row_distribution, column_distribution } )));
  result.info.description = "Automation, MIDI and BPM smoothing control."; // TODO tuning to here
  result.graph_renderer = render_graph;
  result.gui.show_tab_header = false;
  result.force_rerender_on_param_hover = true;
  // TODO state converter  

  gui_dimension dimension({ 1 }, { { 1, 1, 1 } });
  auto section_gui = make_param_section_gui({ 0, 0 }, dimension);
  result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag_basic("{650CEC37-B01B-4EE6-A010-34C2AE1C66B0}", "Main"), section_gui));
  auto& midi_smooth = result.params.emplace_back(make_param(
    make_topo_info_basic("{97BAC4E5-B7EE-43AE-AE6D-B9D4A0A3C94F}", "MIDI Smoothing", param_midi_smooth, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_linear(1, max_other_smoothing_ms, 50, 0, "Ms"),
    make_param_gui_single(section_main, gui_edit_type::hslider, { 0, 0, 1, 1 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  midi_smooth.info.description = "Smoothing MIDI controller changes.";
  auto& bpm_smooth = result.params.emplace_back(make_param(
    make_topo_info_basic("{99B88F8C-6F3B-47E9-932F-D46136252223}", "BPM Smoothing", param_tempo_smooth, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_linear(1, max_other_smoothing_ms, 200, 0, "Ms"),
    make_param_gui_single(section_main, gui_edit_type::hslider, { 0, 1, 1, 1 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  bpm_smooth.info.description = "Smoothing host BPM parameter changes. Affects tempo-synced delay lines.";
  auto& auto_smooth = result.params.emplace_back(make_param(
    make_topo_info_basic("{AF6D2954-3B17-4A32-895B-FB92433761D6}", "Automation Smoothing", param_auto_smooth, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_linear(1, max_auto_smoothing_ms, 1, 0, "Ms"),
    make_param_gui_single(section_main, gui_edit_type::hslider, { 0, 2, 1, 1 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  auto_smooth.info.description = "Smoothing automation parameter changes.";
  return result;
}

}
