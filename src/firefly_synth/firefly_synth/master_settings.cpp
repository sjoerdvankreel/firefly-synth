#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/dsp/graph_engine.hpp>
#include <plugin_base/shared/io_plugin.hpp>

#include <firefly_synth/synth.hpp>

using namespace plugin_base;

namespace firefly_synth {

static int const max_auto_smoothing_ms = 50;
static int const max_other_smoothing_ms = 1000;

enum { section_smooth, section_tuning }; 
enum { override_tuning_off, override_tuning_on };

enum { 
  param_midi_smooth, param_tempo_smooth, param_auto_smooth, 
  param_override_tuning, param_override_tuning_mode, param_count };

// we provide the buttons, everyone else needs to implement it
extern int const master_settings_param_auto_smooth = param_auto_smooth;
extern int const master_settings_param_midi_smooth = param_midi_smooth;
extern int const master_settings_param_tempo_smooth = param_tempo_smooth;
extern int const master_settings_param_override_tuning = param_override_tuning;
extern int const master_settings_param_override_tuning_mode = param_override_tuning_mode;

static std::vector<list_item>
override_tuning_items()
{
  std::vector<list_item> result;
  result.emplace_back("{732C952A-B0B8-41D9-8983-0AC0EAC4127D}", "Off");
  result.emplace_back("{0C55FF9C-B84F-4835-B803-7D0C152EF86C}", "On");
  return result;
}

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

class master_settings_state_converter :
public state_converter
{
  plugin_desc const* const _desc;
public:
  master_settings_state_converter(plugin_desc const* const desc) : _desc(desc) {}
  void post_process_always(load_handler const& handler, plugin_state& new_state) override;
  void post_process_existing(load_handler const& handler, plugin_state& new_state) override {}

  // No params changed, just moved to other module, so we have to post_process everything.
  bool handle_invalid_param_value(
    std::string const& new_module_id, int new_module_slot,
    std::string const& new_param_id, int new_param_slot,
    std::string const& old_value, load_handler const& handler,
    plain_value& new_value) override { return false; }
};

void
master_settings_state_converter::post_process_always(load_handler const& handler, plugin_state& new_state)
{
  std::string old_value;
  auto const& modules = new_state.desc().plugin->modules;
  std::string master_in_id = modules[module_master_in].info.tag.id;

  // All smoothing params moved from Master-In to Master-Settings.
  if (handler.old_version() < plugin_version{ 1, 8, 4 })
  {
    // pick up value from midi smoothing
    if (handler.old_param_value(master_in_id, 0, "{EEA24DB4-220A-4C13-A895-B157BF6158A9}", 0, old_value))
      new_state.set_text_at(module_master_settings, 0, param_midi_smooth, 0, old_value);
    // pick up value from bpm smoothing
    if (handler.old_param_value(master_in_id, 0, "{75053CE4-1543-4595-869D-CC43C6F8CB85}", 0, old_value))
      new_state.set_text_at(module_master_settings, 0, param_tempo_smooth, 0, old_value);
    // pick up value from automation smoothing
    if (handler.old_param_value(master_in_id, 0, "{468FE12E-C1A1-43DF-8D87-ED6C93B2C08D}", 0, old_value))
      new_state.set_text_at(module_master_settings, 0, param_auto_smooth, 0, old_value);
  }
}

module_topo
master_settings_topo(int section, bool is_fx, gui_position const& pos)
{
  std::vector<int> row_distribution = { 1 };
  std::vector<int> column_distribution = { 64, 26, 30, 38, 73, 53 };
  if (is_fx) column_distribution = { 1 };
  module_topo result(make_module(
    make_topo_info("{79A688D2-4223-489A-B2E4-3E78A7975C4D}", true, "Master Settings", "Master Settings", "Master Settings", module_master_settings, 1),
    make_module_dsp(module_stage::input, module_output::none, 0, {}),
      make_module_gui(section, pos, { row_distribution, column_distribution } )));
  result.info.description = "Microtuning support and automation, MIDI and BPM smoothing control.";
  result.gui.tabbed_name = "Master Settings";
  result.graph_renderer = render_graph;
  result.force_rerender_on_param_hover = true;
  result.state_converter_factory = [](auto desc) { return std::make_unique<master_settings_state_converter>(desc); };

  gui_dimension smooth_dimension({ 1, 1 }, { 1, 1 });
  gui_label_edit_cell_split cell_split = gui_label_edit_cell_split::no_split;
  if (is_fx)
  {
    cell_split = gui_label_edit_cell_split::horizontal;
    smooth_dimension = gui_dimension({ 1 }, { gui_dimension::auto_size_all, 1, gui_dimension::auto_size_all, 1, gui_dimension::auto_size_all, 1 });
  }
  auto smooth_section_gui = make_param_section_gui({ 0, 0, 1, is_fx? 1: 4 }, smooth_dimension, cell_split);
  result.sections.emplace_back(make_param_section(section_smooth,
    make_topo_tag_basic("{D02F55AF-1DC8-48F0-B12A-43B47AD6E392}", "Smoothing"), smooth_section_gui));
  auto& midi_smooth = result.params.emplace_back(make_param(
    make_topo_info("{887D373C-D978-48F7-A9E7-70C03A58492A}", true, "MIDI Smoothing", "MIDI Smooth", "MIDI Smooth", param_midi_smooth, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_linear(1, max_other_smoothing_ms, 50, 0, "Ms"),
    make_param_gui_single(section_smooth, gui_edit_type::hslider, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  midi_smooth.info.description = "Smoothing MIDI controller changes.";
  auto& bpm_smooth = result.params.emplace_back(make_param(
    make_topo_info("{AA564CE1-4F1E-44F5-89D9-130F17F4185C}", true, "BPM Smoothing", "BPM Smooth", "BPM Smooth", param_tempo_smooth, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_linear(1, max_other_smoothing_ms, 200, 0, "Ms"),
    make_param_gui_single(section_smooth, gui_edit_type::hslider, { 0, is_fx? 2: 1 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  bpm_smooth.info.description = "Smoothing host BPM parameter changes. Affects tempo-synced delay lines.";
  auto& auto_smooth = result.params.emplace_back(make_param(
    make_topo_info("{852632AB-EF17-47DB-8C5A-3DB32BA78571}", true, "Automation Smoothing", "Auto Smooth", "Auto Smooth", param_auto_smooth, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_linear(1, max_auto_smoothing_ms, 1, 0, "Ms"),
    make_param_gui_single(section_smooth, gui_edit_type::hslider, { is_fx? 0: 1, is_fx? 4: 0, 1, is_fx ? 1: 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::far))));
  auto_smooth.info.description = "Smoothing automation parameter changes.";

  if (is_fx) return result;

  gui_dimension tuning_dimension({ 1, 1 }, { gui_dimension::auto_size_all, 1 });
  auto tuning_section_gui = make_param_section_gui({ 0, 4, 1, 2 }, tuning_dimension, gui_label_edit_cell_split::horizontal);
  result.sections.emplace_back(make_param_section(section_tuning,
    make_topo_tag_basic("{C163A47F-DC37-4D18-B21B-0B71D266B152}", "Tuning"), tuning_section_gui));
  auto& override_tuning = result.params.emplace_back(make_param(
    make_topo_info("{801000F2-D2E5-41B8-BBE3-17435E9512CD}", true, "Override Tuning For Patch", "Override Tuning", "Override Tuning", param_override_tuning, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_item(override_tuning_items(), "Off"),
    make_param_gui_single(section_tuning, gui_edit_type::list, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  override_tuning.info.description = "Enables per-patch MTS-ESP tuning mode override.";
  auto& override_tuning_mode = result.params.emplace_back(make_param(
    make_topo_info("{EC300412-5D8D-49B7-97DD-44C967A76ADC}", true, "Override Tuning Mode For Patch", "Tuning Mode", "Tuning Mode", param_override_tuning_mode, 1),
    make_param_dsp_input(false, param_automate::none), make_domain_item(engine_tuning_mode_items(), "No Tuning"),
    make_param_gui_single(section_tuning, gui_edit_type::autofit_list, { 1, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  override_tuning_mode.info.description = "Selects per-patch MTS-ESP microtuning mode override.";
  override_tuning_mode.gui.bindings.enabled.bind_params({ param_override_tuning }, [](auto const& vs) { return vs[0] != 0; });

  return result;
}

}
