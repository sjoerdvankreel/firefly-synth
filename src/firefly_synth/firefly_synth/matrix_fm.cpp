#include <plugin_base/desc/dims.hpp>
#include <plugin_base/dsp/engine.hpp>
#include <plugin_base/dsp/utility.hpp>
#include <plugin_base/topo/plugin.hpp>
#include <plugin_base/topo/support.hpp>
#include <plugin_base/shared/utility.hpp>
#include <plugin_base/helpers/matrix.hpp>
#include <plugin_base/dsp/graph_engine.hpp>

#include <firefly_synth/synth.hpp>

#include <cmath>
#include <algorithm>

using namespace plugin_base;

namespace firefly_synth { 

enum { section_main };
enum { param_on, param_source, param_target, param_idx, param_dly };
extern int const osc_param_uni_voices;

static int const route_count = 10;

class fm_matrix_engine:
public module_engine { 
public:
  void reset(plugin_block const*) override {}
  void process(plugin_block& block) override {};
};

module_topo 
fm_matrix_topo(int section, gui_colors const& colors, gui_position const& pos, plugin_topo const* plugin)
{
  auto fm_matrix = make_audio_matrix({ &plugin->modules[module_osc] }, 0);

  std::vector<module_dsp_output> outputs;
  for(int r = 0; r < route_count; r++)
    outputs.push_back(make_module_dsp_output(false, make_topo_info(
      "{047875FD-E86C-42E3-BBD3-DB64EFC2ED6F}-" + std::to_string(r), "Modulated", r, max_unison_voices + 1)));
  module_topo result(make_module(
    make_topo_info("{A2E98F59-55BD-40C2-BFEC-7750DF66A58B}", "Osc FM", "FM", true, true, module_fm_matrix, 1),
    make_module_dsp(module_stage::voice, module_output::audio, 0, outputs),
    make_module_gui(section, colors, pos, { 1, 1 })));

  // TODO result.graph_renderer = render_graph;
  // TODO result.graph_engine_factory = make_osc_graph_engine;
  result.gui.tabbed_name = result.info.tag.name;
  result.engine_factory = [](auto const& topo, int, int) { return std::make_unique<fm_matrix_engine>(); };
  result.gui.menu_handler_factory = [](plugin_state* state) { return std::make_unique<tidy_matrix_menu_handler>(
    state, param_on, 0, std::vector<int>({ param_target, param_source })); };

  auto& main = result.sections.emplace_back(make_param_section(section_main,
    make_topo_tag("{F024B7C7-F1E2-4A41-8F6A-78D4AECB7FEE}", "Main"), 
    make_param_section_gui({ 0, 0 }, { { 1 }, { -25, 1, 1, -35, -35 } })));
  main.gui.scroll_mode = gui_scroll_mode::vertical;
  
  auto& on = result.params.emplace_back(make_param(
    make_topo_info("{1CF17D41-4016-43A7-9F88-73E9230E2411}", "On", param_on, route_count),
    make_param_dsp_voice(param_automate::automate), make_domain_toggle(false),
    make_param_gui(section_main, gui_edit_type::toggle, param_layout::vertical, { 0, 0 }, make_label_none())));
  on.gui.tabular = true;
  on.gui.menu_handler_factory = [](plugin_state* state) { return make_matrix_param_menu_handler(state, route_count, 1); };

  auto& source = result.params.emplace_back(make_param(
    make_topo_info("{AC08E9FA-B81B-4A4A-9E62-17112A72D3CA}", "Source", param_source, route_count),
    make_param_dsp_voice(param_automate::none), make_domain_item(fm_matrix.items, ""),
    make_param_gui(section_main, gui_edit_type::list, param_layout::vertical, { 0, 1 }, make_label_none())));
  source.gui.tabular = true;
  source.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });
  source.gui.item_enabled.bind_param({ module_fm_matrix, 0, param_target, gui_item_binding::match_param_slot },
    [fm = fm_matrix.mappings](int other, int self) {
      return fm[self].slot <= fm[other].slot; });

  auto& target = result.params.emplace_back(make_param(
    make_topo_info("{6B8C45F3-C554-4419-9D7E-D8730D279814}", "Target", param_target, route_count),
    make_param_dsp_voice(param_automate::none), make_domain_item(fm_matrix.items, "Osc 2"),
    make_param_gui(section_main, gui_edit_type::list, param_layout::vertical, { 0, 2 }, make_label_none())));
  target.gui.tabular = true;
  target.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });
  target.gui.item_enabled.bind_param({ module_fm_matrix, 0, param_source, gui_item_binding::match_param_slot },
    [fm = fm_matrix.mappings](int other, int self) { 
      return fm[other].slot <= fm[self].slot; });

  auto& idx = result.params.emplace_back(make_param(
    make_topo_info("{FF46E62C-3307-445B-9924-DBDFD9FD3415}", "Idx", param_idx, route_count),
    make_param_dsp_accurate(param_automate::modulate), make_domain_linear(0, 16, 0, 2, ""), // TODO how much
    make_param_gui(section_main, gui_edit_type::knob, param_layout::vertical, { 0, 3 }, make_label_none())));
  idx.gui.tabular = true;
  idx.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  auto& dly = result.params.emplace_back(make_param(
    make_topo_info("{B8ECB799-F66A-4503-808D-44F853E92368}", "Dly", param_dly, route_count),
    make_param_dsp_voice(param_automate::automate), make_domain_linear(0, 5, 0, 2, ""), // TODO how much
    make_param_gui(section_main, gui_edit_type::knob, param_layout::vertical, { 0, 4 }, make_label_none())));
  dly.gui.tabular = true;
  dly.gui.bindings.enabled.bind_params({ param_on }, [](auto const& vs) { return vs[0] != 0; });

  return result;
}

}
