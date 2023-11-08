#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/helpers/dsp.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>

#include <infernal_synth/synth.hpp>
#include <cmath>

using namespace plugin_base;

namespace infernal_synth {

int aux_count = 3;

enum { section_aux, section_midi };
enum { param_aux, param_mod, param_pb, param_pb_range, param_count };

class input_engine :
public module_engine {
public:
  void initialize() override { }
  void process(plugin_block& block) override;
  INF_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(input_engine);
};

module_topo
input_topo(int section, plugin_base::gui_colors const& colors, plugin_base::gui_position const& pos)
{
  module_topo result(make_module(
    make_topo_info("{E22B3B9D-2337-4DE5-AA34-EB3351948D6A}", "In", module_input, 1),
    make_module_dsp(module_stage::input, module_output::cv, 0, {
      make_topo_info("{9D36E713-80F9-49CA-9E81-17E424FF66EE}", "Aux", param_aux, aux_count),
      make_topo_info("{91B915D6-0DCA-4F59-A396-6AF31DA28DBB}", "Mod", param_mod, 1),
      make_topo_info("{EB8CBA31-212A-42EA-956E-69063BF93C58}", "PB", param_pb, 1) }),
    make_module_gui(section, colors, pos, { { 1 }, { 2, 1 } } )));

  result.engine_factory = [](auto const&, int, int) ->
    std::unique_ptr<module_engine> { return std::make_unique<input_engine>(); }; 
    
  result.sections.emplace_back(make_param_section(section_aux,
    make_topo_tag("{BB12B605-4EEF-4FEA-9F2C-FACEEA39644A}", "Aux"),
    make_param_section_gui({ 0, 0 }, gui_dimension({ 1 }, { 1 }))));

  result.params.emplace_back(make_param(
    make_topo_info("{9EC93CE9-6BD6-4D17-97A6-403ED34BBF38}", "Aux", param_aux, aux_count),
    make_param_dsp_accurate(param_automate::both), make_domain_percentage(0, 1, 0, 0, true),
    make_param_gui(section_aux, gui_edit_type::hslider, param_layout::horizontal, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));

  result.sections.emplace_back(make_param_section(section_midi,
    make_topo_tag("{56FD2FEB-3084-4E28-B56C-06D31406EB42}", "MIDI"),
    make_param_section_gui({ 0, 1 }, gui_dimension({ 1 }, { 1, 1, 1 }))));
  
  result.params.emplace_back(make_param(
    make_topo_info("{7696305C-28F3-4C54-A6CA-7C9DB5635153}", "Mod", param_mod, 1),
    make_param_dsp_block(param_automate::modulate), make_domain_percentage(0, 1, 0, 0, true),
    make_param_gui_single(section_midi, gui_edit_type::knob, { 0, 0 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));

  result.params.emplace_back(make_param(
    make_topo_info("{D1B334A6-FA2F-4AE4-97A0-A28DD0C1B48D}", "PB", param_pb, 1),
    make_param_dsp_block(param_automate::modulate), make_domain_percentage(-1, 1, 0, 0, true),
    make_param_gui_single(section_midi, gui_edit_type::knob, { 0, 1 }, 
    make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));

  result.params.emplace_back(make_param(
    make_topo_info("{79B7592A-4911-4B04-8F71-5DD4B2733F4F}", "Range", param_pb_range, 1),
    make_param_dsp_block(param_automate::none), make_domain_step(1, 24, 12, 0),
    make_param_gui_single(section_midi, gui_edit_type::autofit_list, { 0, 2 },
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));

  return result;
}

void
input_engine::process(plugin_block& block)
{
  auto& own_cv = block.state.own_cv;
  auto const& accurate = block.state.own_accurate_automation;
  accurate[param_pb][0].copy_to(block.start_frame, block.end_frame, own_cv[param_pb][0]);
  accurate[param_mod][0].copy_to(block.start_frame, block.end_frame, own_cv[param_mod][0]);
  for(int i = 0; i < aux_count; i++)
    accurate[param_aux][i].copy_to(block.start_frame, block.end_frame, own_cv[param_aux][i]);
}

}
