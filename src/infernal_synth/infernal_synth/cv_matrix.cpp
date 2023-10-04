#include <plugin_base/dsp.hpp>
#include <plugin_base/support.hpp>
#include <plugin_base/engine.hpp>
#include <plugin_base/topo/plugin.hpp>

#include <infernal_synth/synth.hpp>
#include <cmath>
#include <algorithm>

using namespace plugin_base;

namespace infernal_synth {

static int constexpr route_count = 8;

enum { section_main };
enum {
  param_on, param_active,
  param_source, param_source_lfo_index, param_source_env_index,
  param_target, param_target_osc_index, param_target_osc_param,
  param_target_filter_param, param_target_filter_param_osc_gain_index
};

static std::vector<list_item>
source_modules(module_topo const& lfo_topo, module_topo const& env_topo)
{
  std::vector<list_item> result;
  result.emplace_back(lfo_topo);
  result.emplace_back(env_topo);
  return result;
}

static std::vector<list_item>
target_modules(module_topo const& osc_topo, module_topo const& filter_topo)
{
  std::vector<list_item> result;
  result.emplace_back(osc_topo);
  result.emplace_back(filter_topo);
  return result;
}

class cv_matrix_engine:
public module_engine { 
public:
  void initialize() override {}
  void process(process_block& block) override;
  INF_DECLARE_MOVE_ONLY_DEFAULT_CTOR(cv_matrix_engine);
};

module_topo 
cv_matrix_topo(
  module_topo const& lfo_topo,
  module_topo const& env_topo,
  module_topo const& osc_topo,
  module_topo const& filter_topo)
{
  module_topo result(make_module(
    "{1762278E-5B1E-4495-B499-060EE997A8FD}", "Voice CV Matrix", module_cv_matrix, 1, 
    module_stage::voice, module_output::cv, 1,
    gui_layout::single, gui_position { 4, 0 }, gui_dimension { 1, 1 }));
  result.engine_factory = [](int, int, int) -> 
    std::unique_ptr<module_engine> { return std::make_unique<cv_matrix_engine>(); };
  result.sections.emplace_back(make_section(
    "{A19E18F8-115B-4EAB-A3C7-43381424E7AB}", "Main", section_main, gui_position { 0, 0 }, gui_dimension { { 1, 5 }, { 1, 1, 1, 1, 1, 1, 1 } }));
  
  std::vector<int> enabled_params = { param_on, param_active };
  gui_binding_selector enabled_selector = [](auto const& vs, auto const&) { return vs[0] != 0 && vs[1] != 0; };

  result.params.emplace_back(param_toggle(
    "{06512F9B-2B49-4C2E-BF1F-40070065CABB}", "On", param_on, 1, section_main, true,
    param_direction::input,
    gui_label_contents::name, gui_label_align::left, gui_label_justify::center,
    gui_layout::single, gui_position { 0, 0, 1, 7 }));

  auto& active = result.params.emplace_back(param_toggle(
    "{4DF9B283-36FC-4500-ACE6-4AEBF74BA694}", "Active", param_active, route_count, section_main, false,
    param_direction::input,
    gui_label_contents::none, gui_label_align::left, gui_label_justify::center,
    gui_layout::vertical, gui_position { 1, 0 }));
  active.gui.bindings.enabled.params = { param_on };
  active.gui.bindings.enabled.selector = [](auto const& vs, auto const&) { return vs[0] != 0; };

  auto& source = result.params.emplace_back(param_items(
    "{E6D638C0-2337-426D-8C8C-71E9E1595ED3}", "Source", param_source, route_count, section_main, source_modules(lfo_topo, env_topo), "",
    param_direction::input, gui_edit_type::list,
    gui_label_contents::none, gui_label_align::left, gui_label_justify::center,
    gui_layout::vertical, gui_position{ 1, 1 }));
  source.gui.bindings.enabled.params = enabled_params;
  source.gui.bindings.enabled.selector = enabled_selector;

  auto& lfo_index = result.params.emplace_back(param_steps(
    "{5F6A54E9-50E6-4CDE-ACCB-4BA118F06780}", "LFO Index", param_source_lfo_index, route_count, section_main, 0, lfo_topo.info.slot_count - 1, 0,
    param_direction::input, gui_edit_type::list,
    gui_label_contents::none, gui_label_align::left, gui_label_justify::center,
    gui_layout::vertical, gui_position{ 1, 2 }));
  lfo_index.gui.bindings.enabled.params = enabled_params;
  lfo_index.gui.bindings.enabled.selector = enabled_selector;
  lfo_index.gui.bindings.visible.params = { param_source };
  lfo_index.gui.bindings.visible.context = { index_of_item_tag(result.params[param_source].domain.items, module_lfo)};
  lfo_index.gui.bindings.visible.selector = [](auto const& vs, auto const& ctx) { return vs[0] == ctx[0]; };

  auto& env_index = result.params.emplace_back(param_steps(
    "{BA2FB14A-5484-4721-B640-DA26306194A4}", "Env Index", param_source_env_index, route_count, section_main, 0, env_topo.info.slot_count - 1, 0,
    param_direction::input, gui_edit_type::list,
    gui_label_contents::none, gui_label_align::left, gui_label_justify::center,
    gui_layout::vertical, gui_position{ 1, 2 }));
  env_index.gui.bindings.enabled.params = enabled_params;
  env_index.gui.bindings.enabled.selector = enabled_selector;
  env_index.gui.bindings.visible.params = { param_source };
  env_index.gui.bindings.visible.context = { index_of_item_tag(result.params[param_source].domain.items, module_env) };
  env_index.gui.bindings.visible.selector = [](auto const& vs, auto const& ctx) { return vs[0] == ctx[0]; };

  auto& target = result.params.emplace_back(param_items(
    "{94A037CE-F410-4463-8679-5660AFD1582E}", "Target", param_target, route_count, section_main, target_modules(osc_topo, filter_topo), "",
    param_direction::input, gui_edit_type::list,
    gui_label_contents::none, gui_label_align::left, gui_label_justify::center,
    gui_layout::vertical, gui_position{ 1, 3 }));
  target.gui.bindings.enabled.params = enabled_params;
  target.gui.bindings.enabled.selector = enabled_selector;

  auto& osc_index = result.params.emplace_back(param_steps(
    "{79366858-994F-485F-BA1F-34AE3DFD2CEE}", "Osc Index", param_target_osc_index, route_count, section_main, 0, osc_topo.info.slot_count - 1, 0,
    param_direction::input, gui_edit_type::list,
    gui_label_contents::none, gui_label_align::left, gui_label_justify::center,
    gui_layout::vertical, gui_position{ 1, 4 }));
  osc_index.gui.bindings.enabled.params = enabled_params;
  osc_index.gui.bindings.enabled.selector = enabled_selector;
  osc_index.gui.bindings.visible.params = { param_target };
  osc_index.gui.bindings.visible.context = { index_of_item_tag(result.params[param_target].domain.items, module_osc) };
  osc_index.gui.bindings.visible.selector = [](auto const& vs, auto const& ctx) { return vs[0] == ctx[0]; };

  auto& osc_target = result.params.emplace_back(param_items(
    "{28286D1C-6A9D-4CD4-AB70-4A3AFDF7302B}", "Osc Param", param_target_osc_param, route_count, section_main, cv_matrix_target_osc_params(osc_topo), "",
    param_direction::input, gui_edit_type::list,
    gui_label_contents::none, gui_label_align::left, gui_label_justify::center,
    gui_layout::vertical, gui_position{ 1, 5 }));
  osc_target.gui.bindings.enabled.params = enabled_params;
  osc_target.gui.bindings.enabled.selector = enabled_selector;
  osc_target.gui.bindings.visible.params = { param_target };
  osc_target.gui.bindings.visible.context = { index_of_item_tag(result.params[param_target].domain.items, module_osc) };
  osc_target.gui.bindings.visible.selector = [](auto const& vs, auto const& ctx) { return vs[0] == ctx[0]; };

  auto& filter_target = result.params.emplace_back(param_items(
    "{B8098815-BBD5-4171-9AAF-CE4B6645AEE2}", "Filter Param", param_target_filter_param, route_count, section_main, cv_matrix_target_filter_params(filter_topo), "",
    param_direction::input, gui_edit_type::list,
    gui_label_contents::none, gui_label_align::left, gui_label_justify::center,
    gui_layout::vertical, gui_position{ 1, 5 }));
  filter_target.gui.bindings.enabled.params = enabled_params;
  filter_target.gui.bindings.enabled.selector = enabled_selector;
  filter_target.gui.bindings.visible.params = { param_target };
  filter_target.gui.bindings.visible.context = { index_of_item_tag(result.params[param_target].domain.items, module_filter) };
  filter_target.gui.bindings.visible.selector = [](auto const& vs, auto const& ctx) { return vs[0] == ctx[0]; };

  int FILTER_PARAM_OSC_GAIN = 2; // TODO
  auto& osc_gain_index = result.params.emplace_back(param_steps(
    "{FB4EB870-48DD-40D5-9D0E-2E9F0C4E3C48}", "Filter Osc Gain", param_target_filter_param_osc_gain_index, route_count, section_main, 
    0, filter_topo.params[FILTER_PARAM_OSC_GAIN].info.slot_count - 1, 0,
    param_direction::input, gui_edit_type::list,
    gui_label_contents::none, gui_label_align::left, gui_label_justify::center,
    gui_layout::vertical, gui_position{ 1, 6 }));
  osc_gain_index.gui.bindings.enabled.params = enabled_params;
  osc_gain_index.gui.bindings.enabled.selector = enabled_selector;
  osc_gain_index.gui.bindings.visible.params = { param_target, param_target_filter_param };
  osc_gain_index.gui.bindings.visible.context = {
    index_of_item_tag(result.params[param_target].domain.items, module_filter),
    index_of_item_tag(result.params[param_target_filter_param].domain.items, FILTER_PARAM_OSC_GAIN )};
  osc_gain_index.gui.bindings.visible.selector = [](auto const& vs, auto const& ctx) { return vs[0] == ctx[0] && vs[1] == ctx[1]; };

  return result;
}

void
cv_matrix_engine::process(process_block& block)
{
}

}
