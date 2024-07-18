#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/dsp/graph_engine.hpp>

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
  else assert(false);
  std::string partition = state.desc().params[param]->info.name;
  return graph_data(value, false, { partition });
}

module_topo
master_settings_topo(int section, gui_position const& pos)
{
  std::vector<int> row_distribution = { 1 };
  std::vector<int> column_distribution = { 1 };
  module_topo result(make_module(
    make_topo_info("{79A688D2-4223-489A-B2E4-3E78A7975C4D}", true, "Master Settings", "Master Settings", "Master Settings", module_master_settings, 1),
    make_module_dsp(module_stage::input, module_output::none, 0, {}),
      make_module_gui(section, pos, { row_distribution, column_distribution } )));
  result.info.description = "Microtuning support and automation, MIDI and BPM smoothing control.";
  result.gui.tabbed_name = "Master Settings";
  result.graph_renderer = render_graph;
  result.force_rerender_on_param_hover = true;

  auto section_gui = make_param_section_gui({ 0, 0, 1, 1 }, gui_dimension({ 1, 1 }, { gui_dimension::auto_size, 1, gui_dimension::auto_size, 1 }), gui_label_edit_cell_split::horizontal);
  result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag_basic("{D02F55AF-1DC8-48F0-B12A-43B47AD6E392}", "Smoothing"), section_gui));
  auto& midi_smooth = result.params.emplace_back(make_param(
    make_topo_info("{887D373C-D978-48F7-A9E7-70C03A58492A}", true, "MIDI Smoothing", "MIDI Smoothing", "MIDI Smoothing", param_midi_smooth, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_linear(1, max_other_smoothing_ms, 50, 0, "Ms"),
    make_param_gui_single(section_main, gui_edit_type::hslider, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  midi_smooth.info.description = "Smoothing MIDI controller changes.";
  auto& bpm_smooth = result.params.emplace_back(make_param(
    make_topo_info("{AA564CE1-4F1E-44F5-89D9-130F17F4185C}", true, "BPM Smoothing", "BPM Smoothing", "BPM Smoothing", param_tempo_smooth, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_linear(1, max_other_smoothing_ms, 200, 0, "Ms"),
    make_param_gui_single(section_main, gui_edit_type::hslider, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  bpm_smooth.info.description = "Smoothing host BPM parameter changes. Affects tempo-synced delay lines.";
  auto& auto_smooth = result.params.emplace_back(make_param(
    make_topo_info("{852632AB-EF17-47DB-8C5A-3DB32BA78571}", true, "Automation Smoothing", "Automation Smoothing", "Automation Smoothing", param_auto_smooth, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_linear(1, max_auto_smoothing_ms, 1, 0, "Ms"),
    make_param_gui_single(section_main, gui_edit_type::hslider, { 1, 0, 1, 4 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::far))));
  auto_smooth.info.description = "Smoothing automation parameter changes.";

  return result;
}

}
