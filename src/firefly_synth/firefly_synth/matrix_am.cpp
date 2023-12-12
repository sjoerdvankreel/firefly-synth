#include <plugin_base/desc/dims.hpp>
#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/shared/utility.hpp>
#include <plugin_base/helpers/matrix.hpp>

#include <firefly_synth/synth.hpp>

#include <cmath>
#include <algorithm>

using namespace plugin_base;

namespace firefly_synth { 

static int constexpr route_count = 3; // TODO

enum { section_main };
enum { param_on, param_source, param_target, param_amt, param_ring };

class am_matrix_engine:
public module_engine { 
public:
  void reset(plugin_block const*) override {}
  void process(plugin_block& block) override {}
  PB_PREVENT_ACCIDENTAL_COPY_DEFAULT_CTOR(am_matrix_engine);
};

// TODO audio routing audio params, audio routing cv params

module_topo 
am_matrix_topo(int section, gui_colors const& colors, gui_position const& pos, plugin_topo const* plugin)
{
  auto am_matrix = make_audio_matrix({ &plugin->modules[module_osc] });

  module_topo result(make_module(
    make_topo_info("{8024F4DC-5BFC-4C3D-8E3E-C9D706787362}", "AM", "AM", true, module_am_matrix, 1),
    make_module_dsp(module_stage::voice, module_output::none, 0, {}),
    make_module_gui(section, colors, pos, { 1, 1 })));
  result.gui.tabbed_name = result.info.tag.short_name;
  result.gui.menu_handler_factory = [](plugin_state* state) { 
    return std::make_unique<tidy_matrix_menu_handler>(state, param_on, 0, std::vector<int>({ param_target, param_source })); };

  auto& main = result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag("{A48C0675-C020-4D05-A384-EF2B8CA8A066}", "Main"), 
    make_param_section_gui({ 0, 0 }, { { 1 }, { -25, 1, 1, -35, -35 } })));
  main.gui.scroll_mode = gui_scroll_mode::vertical;
  
  auto& on = result.params.emplace_back(make_param(
    make_topo_info("{13B61F71-161B-40CE-BF7F-5022F48D60C7}", "On", param_on, route_count),
    make_param_dsp_block(param_automate::automate), make_domain_toggle(false),
    make_param_gui(section_main, gui_edit_type::toggle, param_layout::vertical, { 0, 0 }, gui_label_contents::none, make_label_none())));
  on.gui.tabular = true;

  auto& source = result.params.emplace_back(make_param(
    make_topo_info("{1D8F3294-2463-470D-853B-561E8228467A}", "Source", param_source, route_count),
    make_param_dsp_block(param_automate::none), make_domain_item(am_matrix.items, ""),
    make_param_gui(section_main, gui_edit_type::list, param_layout::vertical, { 0, 1 }, gui_label_contents::value, make_label_none())));
  source.gui.tabular = true;
  source.gui.submenu = am_matrix.submenu;
  source.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });
  source.gui.item_enabled.bind_param({ module_am_matrix, 0, param_target, gui_item_binding::match_param_slot },
    [am = am_matrix.mappings](int other, int self) { return am[self].slot <= am[other].slot; });

  auto& target = result.params.emplace_back(make_param(
    make_topo_info("{1AF0E66A-ADB5-40F4-A4E1-9F31941171E2}", "Target", param_target, route_count),
    make_param_dsp_block(param_automate::none), make_domain_item(am_matrix.items, "Osc 1"),
    make_param_gui(section_main, gui_edit_type::list, param_layout::vertical, { 0, 2 }, gui_label_contents::value, make_label_none())));
  target.gui.tabular = true;
  target.gui.submenu = am_matrix.submenu;
  target.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });
  target.gui.item_enabled.bind_param({ module_am_matrix, 0, param_source, gui_item_binding::match_param_slot },
    [am = am_matrix.mappings](int other, int self) { return am[other].slot <= am[self].slot; });

  auto& amount = result.params.emplace_back(make_param(
    make_topo_info("{A1A7298E-542D-4C2F-9B26-C1AF7213D095}", "Amt", param_amt, route_count),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(0, 1, 0, 0, true),
    make_param_gui(section_main, gui_edit_type::knob, param_layout::vertical, { 0, 3 }, gui_label_contents::value, make_label_none())));
  amount.gui.tabular = true;
  amount.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  auto& ring = result.params.emplace_back(make_param(
    make_topo_info("{3DF51ADC-9882-4F95-AF4E-5208EB14E645}", "Ring", param_ring, route_count),
    make_param_dsp_accurate(param_automate::automate_modulate), make_domain_percentage(0, 1, 0, 0, true),
    make_param_gui(section_main, gui_edit_type::knob, param_layout::vertical, { 0, 4 }, gui_label_contents::value, make_label_none())));
  ring.gui.tabular = true;
  ring.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  // TODO result.graph_renderer = render_graph;
  result.rerender_on_param_hover = true;
  result.engine_factory = [](auto const& topo, int, int) -> std::unique_ptr<module_engine> { return std::make_unique<am_matrix_engine>(); };

  return result;
}

}
