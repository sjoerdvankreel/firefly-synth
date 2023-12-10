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

enum { section_main, section_pitch };
enum { porta_off, porta_on, porta_auto };
enum { param_mode, param_porta, param_porta_sync, param_note, param_cent, param_pitch, param_pb, param_count };

extern int const voice_in_param_pb = param_pb;
extern int const voice_in_param_note = param_note;
extern int const voice_in_param_cent = param_cent;
extern int const voice_in_param_pitch = param_pitch;

static std::vector<list_item>
mode_items()
{
  std::vector<list_item> result;
  result.emplace_back("{88F746C4-1A70-4A64-A11D-584D87D3059C}", "Poly");
  result.emplace_back("{6ABA8E48-F284-40A4-A0E2-C263B536D493}", "Mono");
  return result;
}

static std::vector<list_item>
porta_items()
{
  std::vector<list_item> result;
  result.emplace_back("{51C360E5-967A-4218-B375-5052DAC4FD02}", "Porta Off");
  result.emplace_back("{112A9728-8564-469E-95A7-34FE5CC7C8FC}", "Porta On");
  result.emplace_back("{0E3AF80A-F242-4176-8C72-C0C91D72AEBB}", "Porta Auto");
  return result;
}

class voice_in_engine :
public module_engine {
public:
  void reset(plugin_block const*) override {}
  void process(plugin_block& block) override {}
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(voice_in_engine);
};

static graph_data
render_graph(plugin_state const& state, param_topo_mapping const& mapping)
{
  if (mapping.param_index != param_cent)  return graph_data();
  float value = state.get_plain_at(mapping).real();
  return graph_data(value, true);
}

module_topo
voice_in_topo(int section, gui_colors const& colors, gui_position const& pos)
{
  module_topo result(make_module(
    make_topo_info("{524138DF-1303-4961-915A-3CAABA69D53A}", "Voice In", "V.In", true, module_voice_in, 1),
    make_module_dsp(module_stage::voice, module_output::none, 0, {}),
    make_module_gui(section, colors, pos, { { 1 }, { 1, 1 } } )));
  result.graph_renderer = render_graph;
  result.rerender_on_param_hover = true;
  result.gui.menu_handler_factory = make_cv_routing_menu_handler;
  result.engine_factory = [](auto const&, int, int) -> std::unique_ptr<module_engine> { return std::make_unique<voice_in_engine>(); };

  result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag("{C85AA7CC-FBD1-4631-BB7A-831A2E084E9E}", "Main"),
    make_param_section_gui({ 0, 0 }, gui_dimension({ 1 }, { gui_dimension::auto_size, gui_dimension::auto_size, 1 }))));

  result.params.emplace_back(make_param(
    make_topo_info("{F26D6913-63E8-4A23-97C0-9A17D859ED93}", "Mode", param_mode, 1),
    make_param_dsp_block(param_automate::automate), make_domain_item(mode_items(), ""),
    make_param_gui_single(section_main, gui_edit_type::autofit_list, { 0, 0 }, gui_label_contents::name, make_label_none())));

  result.params.emplace_back(make_param(
    make_topo_info("{586BEE16-430A-483E-891B-48E89C4B8FC1}", "Porta", param_porta, 1),
    make_param_dsp_block(param_automate::automate), make_domain_item(porta_items(), ""),
    make_param_gui_single(section_main, gui_edit_type::autofit_list, { 0, 1 }, gui_label_contents::name, make_label_none())));

  auto& sync = result.params.emplace_back(make_param(
    make_topo_info("{FE70E21D-2104-4EB6-B852-6CD9690E5F72}", "Sync", param_porta_sync, 1),
    make_param_dsp_block(param_automate::automate), make_domain_toggle(false),
    make_param_gui_single(section_main, gui_edit_type::toggle, { 0, 2 }, gui_label_contents::none, 
      make_label(gui_label_contents::name, gui_label_align::left, gui_label_justify::center))));
  sync.gui.bindings.enabled.bind_params({ param_porta }, [](auto const& vs) { return vs[0] != porta_off; });

  result.sections.emplace_back(make_param_section(section_pitch,
    make_topo_tag("{3EB05593-E649-4460-929C-993B6FB7BBD3}", "Pitch"),
    make_param_section_gui({ 0, 1 }, gui_dimension({ 1 }, { gui_dimension::auto_size, 1 }))));
  
  auto& note = result.params.emplace_back(make_param(
    make_topo_info("{CB6D7BC8-5DE6-4A84-97C9-4E405A96E0C8}", "Note", param_note, 1),
    make_param_dsp_block(param_automate::automate), make_domain_item(make_midi_note_list(), "C4"),
    make_param_gui_single(section_pitch, gui_edit_type::autofit_list, { 0, 0 }, gui_label_contents::name, make_label_none())));
  note.gui.submenu = make_midi_note_submenu();

  result.params.emplace_back(make_param(
    make_topo_info("{57A908CD-ED0A-4FCD-BA5F-92257175A9DE}", "Cent", param_cent, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(-1, 1, 0, 0, false),
    make_param_gui_single(section_pitch, gui_edit_type::hslider, { 0, 1 }, gui_label_contents::name,
      make_label(gui_label_contents::value, gui_label_align::left, gui_label_justify::center))));

  result.params.emplace_back(make_param(
    make_topo_info("{034AE513-9AB6-46EE-8246-F6ECCC11CAE0}", "Pitch", param_pitch, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_linear(-128, 128, 0, 0, ""),
    make_param_gui_none()));

  result.params.emplace_back(make_param(
    make_topo_info("{BF20BA77-A162-401B-9F32-92AE34841AB2}", "PB", param_pb, 1),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(-1, 1, 0, 0, true),
    make_param_gui_none()));

  return result;
}

}
