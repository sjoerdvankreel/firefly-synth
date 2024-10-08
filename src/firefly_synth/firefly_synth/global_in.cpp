#include <plugin_base/helpers/dsp.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/desc/param.hpp>
#include <plugin_base/shared/state.hpp>
#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/dsp/graph_engine.hpp>

#include <firefly_synth/synth.hpp>
#include <cmath>

using namespace plugin_base;

namespace firefly_synth {

static int const aux_count = 6;

enum { section_aux, section_linked };
enum { output_aux, output_mod, output_pb };
enum { param_aux, param_mod, param_pb, param_pb_range, param_count };

// we provide the buttons, everyone else needs to implement it
extern int const global_in_param_pb_range = param_pb_range;

class global_in_engine :
public module_engine {
public:
  void reset_audio(plugin_block const*,
    std::vector<note_event> const* in_notes,
    std::vector<note_event>* out_notes) override {}
  void process_audio(plugin_block& block,
    std::vector<note_event> const* in_notes,
    std::vector<note_event>* out_notes) override;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(global_in_engine);
};

static graph_data
render_graph(
  plugin_state const& state, graph_engine* engine, int param, 
  param_topo_mapping const& mapping, std::vector<mod_out_custom_state> const& custom_outputs)
{
  if (mapping.param_index == param_pb_range)
    return graph_data(graph_data_type::na, {});
  float value = state.get_plain_at(mapping).real();
  bool bipolar = mapping.param_index == param_pb;
  std::string partition = state.desc().params[param]->info.name;
  return graph_data(value, bipolar, { partition });
}

module_topo
global_in_topo(int section, bool is_fx, gui_position const& pos)
{
  std::vector<int> row_distribution = { 1 };
  std::vector<int> column_distribution = { 37, 27, 26, 44, 24, 22, 28, 76 };
  module_topo result(make_module(
    make_topo_info_basic("{E22B3B9D-2337-4DE5-AA34-EB3351948D6A}", "Global", module_global_in, 1),
    make_module_dsp(module_stage::input, module_output::cv, 0, {
      make_module_dsp_output(true, -1, make_topo_info_basic("{9D36E713-80F9-49CA-9E81-17E424FF66EE}", "Aux", output_aux, aux_count)),
      make_module_dsp_output(true, -1, make_topo_info("{91B915D6-0DCA-4F59-A396-6AF31DA28DBB}", true, "Mod Wheel", "Mod", "Mod", output_mod, 1)),
      make_module_dsp_output(true, -1, make_topo_info("{EB8CBA31-212A-42EA-956E-69063BF93C58}", true, "Pitch Bend", "PB", "PB", output_pb, 1)) }),
      make_module_gui(section, pos, { row_distribution, column_distribution } )));
  result.gui.tabbed_name = "Global";  
  result.info.description = "Global CV module with MIDI-linked modwheel and pitchbend, and some additional freely-assignable parameters.";

  result.graph_renderer = render_graph;
  result.gui.force_rerender_graph_on_param_hover = true;
  result.gui.rerender_graph_on_modulation = false;
  result.gui.menu_handler_factory = make_cv_routing_menu_handler;
  result.engine_factory = [](auto const&, int, int) { return std::make_unique<global_in_engine>(); };

  auto section_aux_gui = make_param_section_gui({ 0, 0, 1, 5 }, gui_dimension({ 1 }, {
      gui_dimension::auto_size_all, 1,
      gui_dimension::auto_size_all, 1,
      gui_dimension::auto_size_all, 1,
      gui_dimension::auto_size_all, 1,
      gui_dimension::auto_size_all, 1,
      gui_dimension::auto_size_all, 1 }),
      gui_label_edit_cell_split::horizontal);
  result.sections.emplace_back(make_param_section(section_aux,
    make_topo_tag_basic("{BB12B605-4EEF-4FEA-9F2C-FACEEA39644A}", "Aux"), section_aux_gui));
  auto& aux = result.params.emplace_back(make_param(
    make_topo_info_basic("{9EC93CE9-6BD6-4D17-97A6-403ED34BBF38}", "Aux", param_aux, aux_count),
    make_param_dsp_accurate(param_automate::modulate), make_domain_percentage_identity(0, 0, true),
    make_param_gui(section_aux, gui_edit_type::knob, param_layout::parent_grid, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  aux.info.description = "Auxilliary controls to be used through automation and the CV matrices.";
  aux.gui.alternate_drag_output_id = result.dsp.outputs[output_aux].info.tag.id;
  aux.gui.display_formatter = [](auto const& desc) { 
    return desc.info.slot == 0 ? desc.info.name : std::to_string(desc.info.slot + 1); };

  auto linked_gui = make_param_section_gui(
    { 0, 5, 1, 3 }, gui_dimension({ 1 }, { 
      gui_dimension::auto_size_all, gui_dimension::auto_size_all, gui_dimension::auto_size_all, 
      gui_dimension::auto_size_all, gui_dimension::auto_size_all, 1 }), gui_label_edit_cell_split::horizontal);
  result.sections.emplace_back(make_param_section(section_linked,
    make_topo_tag_basic("{56FD2FEB-3084-4E28-B56C-06D31406EB42}", "Linked"), linked_gui));
  auto& mod_wheel = result.params.emplace_back(make_param(
    make_topo_info("{7696305C-28F3-4C54-A6CA-7C9DB5635153}", true, "Mod Wheel", "Mod Wheel", "Mod", param_mod, 1),
    make_param_dsp_midi({ module_midi, 0, 1 }), make_domain_percentage_identity(0, 0, true),
    make_param_gui_single(section_linked, gui_edit_type::knob, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  mod_wheel.info.description = "Linked to MIDI mod wheel, updates on incoming MIDI events.";
  mod_wheel.gui.alternate_drag_output_id = result.dsp.outputs[output_mod].info.tag.id;
  auto& pitch_bend = result.params.emplace_back(make_param(
    make_topo_info("{D1B334A6-FA2F-4AE4-97A0-A28DD0C1B48D}", true, "Pitch Bend", "Pitch Bend", "PB", param_pb, 1),
    make_param_dsp_midi({ module_midi, 0, midi_source_pb }), make_domain_percentage(-1, 1, 0, 0, true),
    make_param_gui_single(section_linked, gui_edit_type::knob, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  pitch_bend.info.description = "Linked to MIDI pitch bend, updates on incoming MIDI events.";
  pitch_bend.gui.alternate_drag_output_id = result.dsp.outputs[output_pb].info.tag.id;
  auto& pb_range = result.params.emplace_back(make_param(
    make_topo_info("{79B7592A-4911-4B04-8F71-5DD4B2733F4F}", true, "Pitch Bend Range", "Range", "PB Range", param_pb_range, 1),
    make_param_dsp_block(param_automate::automate), make_domain_step(1, 24, 12, 0),
    make_param_gui_single(section_linked, gui_edit_type::list, { 0, 4 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::near))));
  pb_range.info.description = "Pitch bend range. Together with Pitch Bend this affects the base pitch of all oscillators.";
  pb_range.gui.bindings.enabled.bind_slot([is_fx](int) { return !is_fx; });
  return result;
}

void
global_in_engine::process_audio(
  plugin_block& block,
  std::vector<note_event> const* in_notes,
  std::vector<note_event>* out_notes)
{
  auto& own_cv = block.state.own_cv;  
  auto const& accurate = block.state.own_accurate_automation;
  accurate[param_pb][0].copy_to(block.start_frame, block.end_frame, own_cv[output_pb][0]);
  accurate[param_mod][0].copy_to(block.start_frame, block.end_frame, own_cv[output_mod][0]);
  for(int i = 0; i < aux_count; i++)
    accurate[param_aux][i].copy_to(block.start_frame, block.end_frame, own_cv[output_aux][i]);
}

}
