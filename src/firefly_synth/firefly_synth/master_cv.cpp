#include <plugin_base/helpers/dsp.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/shared/state.hpp>
#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/dsp/graph_engine.hpp>

#include <firefly_synth/synth.hpp>
#include <cmath>

using namespace plugin_base;

namespace firefly_synth {

int aux_count = 3;
enum { section_aux, section_linked };
enum { output_aux, output_mod, output_pb };
enum { param_aux, param_mod, param_pb, param_pb_range, param_count };
extern int const master_cv_param_pb_range = param_pb_range;

class master_cv_engine :
public module_engine {
public:
  void reset(plugin_block const*) override {}
  void process(plugin_block& block) override;
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(master_cv_engine);
};

static graph_data
render_graph(plugin_state const& state, param_topo_mapping const& mapping)
{
  if (mapping.param_index == param_pb_range)
    return graph_data();
  float value = state.get_plain_at(mapping).real();
  bool bipolar = mapping.param_index == param_pb;
  return graph_data(value, bipolar);
}

module_topo
master_cv_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos)
{
  module_topo result(make_module(
    make_topo_info("{E22B3B9D-2337-4DE5-AA34-EB3351948D6A}", "Master", "Mst", true, module_master_cv, 1),
    make_module_dsp(module_stage::input, module_output::cv, 0, {
      make_module_dsp_output(true, make_topo_info("{9D36E713-80F9-49CA-9E81-17E424FF66EE}", "Aux", output_aux, aux_count)),
      make_module_dsp_output(true, make_topo_info("{91B915D6-0DCA-4F59-A396-6AF31DA28DBB}", "Mod", output_mod, 1)),
      make_module_dsp_output(true, make_topo_info("{EB8CBA31-212A-42EA-956E-69063BF93C58}", "PB", output_pb, 1)) }),
    make_module_gui(section, colors, pos, { { 1 }, { 1, 1 } } )));
  result.gui.menu_handler_factory = make_cv_routing_menu_handler;

  result.graph_renderer = render_graph;
  result.rerender_on_param_hover = true;
  result.engine_factory = [](auto const&, int, int) ->
    std::unique_ptr<module_engine> { return std::make_unique<master_cv_engine>(); };

  result.sections.emplace_back(make_param_section(section_aux,
    make_topo_tag("{BB12B605-4EEF-4FEA-9F2C-FACEEA39644A}", "Aux"),
    make_param_section_gui({ 0, 0 }, gui_dimension({ 1 }, { 1 }))));

  result.params.emplace_back(make_param(
    make_topo_info("{9EC93CE9-6BD6-4D17-97A6-403ED34BBF38}", "Aux", param_aux, aux_count),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(0, 1, 0, 0, true),
    make_param_gui(section_aux, gui_edit_type::knob, param_layout::horizontal, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));

  result.sections.emplace_back(make_param_section(section_linked,
    make_topo_tag("{56FD2FEB-3084-4E28-B56C-06D31406EB42}", "Linked"),
    make_param_section_gui({ 0, 1 }, gui_dimension({ 1 }, { 2, 2, 3 }))));
  
  result.params.emplace_back(make_param(
    make_topo_info("{7696305C-28F3-4C54-A6CA-7C9DB5635153}", "Mod", param_mod, 1),
    make_param_dsp_midi({ module_midi, 0, 1 }), make_domain_percentage(0, 1, 0, 0, true),
    make_param_gui_single(section_linked, gui_edit_type::knob, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));

  result.params.emplace_back(make_param(
    make_topo_info("{D1B334A6-FA2F-4AE4-97A0-A28DD0C1B48D}", "PB", param_pb, 1),
    make_param_dsp_midi({ module_midi, 0, midi_source_pb }), make_domain_percentage(-1, 1, 0, 0, true),
    make_param_gui_single(section_linked, gui_edit_type::knob, { 0, 1 },
    make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));

  result.params.emplace_back(make_param(
    make_topo_info("{79B7592A-4911-4B04-8F71-5DD4B2733F4F}", "Range", param_pb_range, 1),
    make_param_dsp_block(param_automate::none), make_domain_step(1, 24, 12, 0),
    make_param_gui_single(section_linked, gui_edit_type::autofit_list, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));

  return result;
}

void
master_cv_engine::process(plugin_block& block)
{
  auto& own_cv = block.state.own_cv;  
  auto const& accurate = block.state.own_accurate_automation;
  accurate[param_pb][0].copy_to(block.start_frame, block.end_frame, own_cv[output_pb][0]);
  accurate[param_mod][0].copy_to(block.start_frame, block.end_frame, own_cv[output_mod][0]);
  for(int i = 0; i < aux_count; i++)
    accurate[param_aux][i].copy_to(block.start_frame, block.end_frame, own_cv[output_aux][i]);
}

}
