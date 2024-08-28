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

enum { section_tuning, section_preset, section_smoothing }; 
enum { param_tuning_mode, param_preset, param_midi_smooth, param_tempo_smooth, param_auto_smooth, param_count };

// we provide the buttons, everyone else needs to implement it
extern int const master_settings_param_auto_smooth = param_auto_smooth;
extern int const master_settings_param_midi_smooth = param_midi_smooth;
extern int const master_settings_param_tempo_smooth = param_tempo_smooth;
extern int const master_settings_param_tuning_mode = param_tuning_mode;

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
master_settings_topo(int section, gui_position const& pos, bool is_fx, plugin_base::plugin_topo const* plugin)
{
  std::vector<int> row_distribution = { 1 };
  std::vector<int> column_distribution = { -153, -200, 1 };
  module_topo result(make_module( 
    make_topo_info_basic("{7F400614-E996-4B02-9B78-80E22F1C44A4}", "Master", module_master_settings, 1),
    make_module_dsp(module_stage::input, module_output::none, 0, {}),
      make_module_gui(section, pos, { row_distribution, column_distribution } )));
  result.info.description = "Automation, MIDI and BPM smoothing control and microtuning mode.";
  result.graph_renderer = render_graph;
  result.gui.show_tab_header = false;
  result.force_rerender_on_param_hover = true;

  result.sections.emplace_back(make_param_section(section_tuning,
    make_topo_tag_basic("{650CEC37-B01B-4EE6-A010-34C2AE1C66B0}", "Tuning"), 
    make_param_section_gui({ 0, 0 }, { 1, 1 })));
  auto& tuning_mode = result.params.emplace_back(make_param(
    make_topo_info_basic("{28C619C2-C04E-4BD6-8D84-89667E1A5659}", "Tuning", param_tuning_mode, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_item(engine_tuning_mode_items(), "On Note Before Mod"),
    make_param_gui_single(section_tuning, gui_edit_type::autofit_list, { 0, 0, 1, 1 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  tuning_mode.info.is_per_instance = true;
  tuning_mode.info.description = "Microtuning mode.";
  tuning_mode.gui.bindings.enabled.bind_slot([is_fx](int) { return !is_fx; });
  
  result.sections.emplace_back(make_param_section(section_preset,
    make_topo_tag_basic("{C13C6219-D905-496B-9583-DBAD4626DC0F}", "Preset"),
    make_param_section_gui({ 0, 1 }, { 1, 1 })));
  auto& preset = result.params.emplace_back(make_param(
    make_topo_info_basic("{008EAC13-807E-41F7-9436-3179FD20913E}", "Preset", param_preset, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_item(plugin->preset_list(), ""),
    make_param_gui_single(section_preset, gui_edit_type::autofit_list, { 0, 0, 1, 1 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  preset.info.is_per_instance = true;
  preset.gui.is_preset_selector = true;
  preset.info.description = "Factory preset.";
  preset.gui.submenu = plugin->preset_submenu();

  result.sections.emplace_back(make_param_section(section_smoothing,
    make_topo_tag_basic("{34720C70-C18C-4615-B5DA-F0CEF8FBC0AF}", "Smoothing"),
    make_param_section_gui({ 0, 2 }, { { 1 }, { gui_dimension::auto_size, 1, gui_dimension::auto_size, 1, gui_dimension::auto_size, 1 } }, 
      gui_label_edit_cell_split::horizontal)));
  auto& midi_smooth = result.params.emplace_back(make_param(
    make_topo_info_basic("{97BAC4E5-B7EE-43AE-AE6D-B9D4A0A3C94F}", "MIDI Smoothing", param_midi_smooth, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_linear(1, max_other_smoothing_ms, 50, 0, "Ms"),
    make_param_gui_single(section_smoothing, gui_edit_type::hslider, { 0, 0, 1, 1 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  midi_smooth.info.description = "Smoothing MIDI controller changes.";
  midi_smooth.info.is_per_instance = true;
  auto& bpm_smooth = result.params.emplace_back(make_param(
    make_topo_info_basic("{99B88F8C-6F3B-47E9-932F-D46136252223}", "BPM Smoothing", param_tempo_smooth, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_linear(1, max_other_smoothing_ms, 200, 0, "Ms"),
    make_param_gui_single(section_smoothing, gui_edit_type::hslider, { 0, 2, 1, 1 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  bpm_smooth.info.description = "Smoothing host BPM parameter changes. Affects tempo-synced delay lines.";
  bpm_smooth.info.is_per_instance = true;
  auto& auto_smooth = result.params.emplace_back(make_param(
    make_topo_info_basic("{AF6D2954-3B17-4A32-895B-FB92433761D6}", "Automation Smoothing", param_auto_smooth, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_linear(1, max_auto_smoothing_ms, 1, 0, "Ms"),
    make_param_gui_single(section_smoothing, gui_edit_type::hslider, { 0, 4, 1, 1 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  auto_smooth.info.description = "Smoothing automation parameter changes.";
  auto_smooth.info.is_per_instance = true;
  return result;
}

}
